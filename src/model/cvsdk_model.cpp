// Copyright 2018 The SAF Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Intel Inference Engine model

#include "model/cvsdk_model.h"

#include <ie_plugin_config.hpp>
#include <vpu/vpu_plugin_config.hpp>
#include "cvsdk/common.hpp"

#include <ie_extension.h>
#include "cvsdk/ext_list.hpp"

#include "common/context.h"
#include "model/model_manager.h"
#include "utils/cv_utils.h"

using namespace InferenceEngine::details;
using namespace InferenceEngine;

class CVSDKGlobal {
 public:
  CVSDKGlobal() {
    InferenceEngine::PluginDispatcher dispatcher({"", "", ""});
    InferenceEngine::InferenceEnginePluginPtr engine_;

    engine_ = dispatcher.getPluginByDevice("CPU");
  }
};

namespace {
static CVSDKGlobal cvsdk_global;
}

CVSDKModel::CVSDKModel(const ModelDesc& model_desc, Shape input_shape,
                       size_t batch_size)
    : Model(model_desc, input_shape, batch_size) {}

void CVSDKModel::Load() { Initialize(model_desc_); }

std::unordered_map<std::string, std::vector<cv::Mat>> CVSDKModel::Evaluate(
    const std::unordered_map<std::string, std::vector<cv::Mat>>& input_map,
    const std::vector<std::string>& output_layer_names) {
  InferenceEngine::InputsDataMap inputInfo(
      network_builder_.getNetwork().getInputsInfo());
  auto item = inputInfo.begin();
  // Collect images data ptrs
  std::vector<std::shared_ptr<std::vector<unsigned char>>> vreader;
  for (auto& i : input_map.begin()->second) {
    // Getting image data
    auto data = OCVReaderGetData(i, item->second->getDims()[0],
                                 item->second->getDims()[1]);
    if (data != nullptr) {
      vreader.push_back(data);
    }
  }
  if (vreader.empty())
    throw std::logic_error("Valid input images were not found!");

  // Setting batch size using image count
  network_builder_.getNetwork().setBatchSize(vreader.size());

  // Fill input tensor with images in BGR
  size_t num_chanels = input_->dims()[2];
  size_t image_size = input_->dims()[1] * input_->dims()[0];

  // Iterate over all input images
  for (size_t image_id = 0; image_id < vreader.size(); ++image_id) {
    // Iterate over all pixel in image (b,g,r)
    for (size_t pid = 0; pid < image_size; pid++) {
      // Iterate over all channels
      for (size_t ch = 0; ch < num_chanels; ++ch) {
        // [images stride + channels stride + pixel id ] all in bytes
        input_->data()[image_id * image_size * num_chanels + ch * image_size +
                       pid] = (*vreader.at(image_id))[pid * num_chanels + ch];
      }
    }
  }

  // Perform inference
  InferenceEngine::ResponseDesc resp;
  InferenceEngine::IInferRequest::Ptr request;
  network_->CreateInferRequest(request, &resp);
  request->SetBlob(network_input_name_.c_str(),
                   input_blobs_[network_input_name_], &resp);
  InferenceEngine::StatusCode status = request->Infer(&resp);
  if (status != InferenceEngine::OK) {
    throw std::logic_error(resp.msg);
  }

  // Grab the output blobs
  request->GetBlob(network_output_name_.c_str(),
                   output_blobs_[network_output_name_], &resp);
  std::unordered_map<std::string, std::vector<cv::Mat>> output_layers;
  for (const auto& layer : output_layer_names) {
    for (decltype(batch_size_) batch_idx = 0; batch_idx < batch_size_;
         ++batch_idx) {
      output_layers[layer].push_back(
          GetLayerOutput(layer, batch_idx, output_blobs_));
    }
  }
  return output_layers;
}

cv::Mat CVSDKModel::GetLayerOutput(
    const std::string& layer_name, int batch_idx,
    const InferenceEngine::BlobMap& outputBlobs) const {
  // Postprocess output blobs
  const TBlob<float>::Ptr fOutput =
      std::dynamic_pointer_cast<TBlob<float>>(outputBlobs.begin()->second);

  TBlob<float>& input = *fOutput;
  // The last layer is often 2-dimensional (batch, 1D array of probabilities)
  // Intermediate layers are always 4-dimensional
  if (1) {
    return BlobToMat2d(input, batch_idx);
  } else {
    LOG(FATAL)
        << "Error, only 2D and 4D feature vectors are supported at this time";
  }
  return cv::Mat();
}

cv::Mat CVSDKModel::BlobToMat2d(TBlob<float>& input, int batch_idx) const {
  size_t input_rank = input.dims().size();
  if (!input_rank || !input.dims().at(input_rank - 1))
    THROW_IE_EXCEPTION << "Input blob has incorrect dimensions!";
  size_t batchSize = input.dims().at(input_rank - 1);
  CHECK(batchSize == batch_size_) << "Incorrect batch size";
  size_t dataSize = input.size() / batchSize;

  std::vector<int> mat_size({1, (int)dataSize});
  float* data = input.data();
  cv::Mat ret_mat(mat_size, CV_32F);
  memcpy(ret_mat.data, data + dataSize * batch_idx, dataSize * sizeof(float));
  return ret_mat;
}
