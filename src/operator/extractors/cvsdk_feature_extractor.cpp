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

// Feature extractor using CVSDK

#include "operator/extractors/cvsdk_feature_extractor.h"

#include <ie_extension.h>
#include <ie_plugin_config.hpp>
#include <vpu/vpu_plugin_config.hpp>
#include "cvsdk/common.hpp"
#include "cvsdk/ext_list.hpp"

#include "common/context.h"
#include "model/model_manager.h"

#include "utils/cv_utils.h"
using namespace InferenceEngine::details;
using namespace InferenceEngine;

bool CVSDKCNNFeatureExtractor::Init() {
  Initialize(model_desc_);

  LOG(INFO) << "CVSDKCNNFeatureExtractor initialized";
  return true;
}

void GetFeatures(TBlob<float>& input,
                 std::vector<std::vector<double>>& features) {
  size_t input_rank = input.dims().size();
  if (!input_rank || !input.dims().at(input_rank - 1))
    THROW_IE_EXCEPTION << "Input blob has incorrect dimensions!";
  size_t batchSize = input.dims().at(input_rank - 1);
  size_t dataSize = input.size() / batchSize;

  for (size_t i = 0; i < batchSize; i++) {
    size_t offset = i * (input.size() / batchSize);
    float* batchData = input.data();
    batchData += offset;

    std::vector<double> feature(batchData, batchData + dataSize);
    features.push_back(feature);
  }
}

void CVSDKCNNFeatureExtractor::Extract(
    const cv::Mat& image, const std::vector<Rect>& bboxes,
    std::vector<std::vector<double>>& features) {
  for (auto& i : bboxes) {
    std::vector<Rect> bboxes_individual;
    bboxes_individual.push_back(i);
    std::vector<std::vector<double>> features_individual;
    ExtractBatch(image, bboxes_individual, features_individual);
    CHECK(features_individual.size() == 1);
    features.push_back(features_individual.at(0));
  }
}

void CVSDKCNNFeatureExtractor::ExtractBatch(
    const cv::Mat& image, const std::vector<Rect>& bboxes,
    std::vector<std::vector<double>>& features) {
  InferenceEngine::InputsDataMap inputInfo(
      network_builder_.getNetwork().getInputsInfo());
  auto item = inputInfo.begin();
  /** Collect images data ptrs **/
  std::vector<std::shared_ptr<std::vector<unsigned char>>> vreader;
  for (auto& i : bboxes) {
    int x = i.px;
    int y = i.py;
    int w = i.width;
    int h = i.height;
    CHECK((x >= 0) && (y >= 0) && (x + w <= image.cols) &&
          (y + h <= image.rows));
    cv::Rect roi(x, y, w, h);
    auto bbox_image = image(roi);
    cv::Mat mean_image(bbox_image.size(), bbox_image.type(),
                       cv::Scalar(104, 117, 124));
    cv::Mat bbox_image_1;
    cv::subtract(bbox_image, mean_image, bbox_image_1);

    /** Getting image data **/
    auto data = OCVReaderGetData(bbox_image_1, item->second->getDims()[0],
                                 item->second->getDims()[1]);
    if (data.get() != nullptr) {
      vreader.push_back(data);
    }
  }
  if (vreader.empty())
    throw std::logic_error("Valid input images were not found!");

  /** Seting batch size using image count **/
  network_builder_.getNetwork().setBatchSize(vreader.size());

  /** Fill input tensor with images. First b channel, then g and r channels **/
  size_t num_chanels = input_->dims()[2];
  size_t image_size = input_->dims()[1] * input_->dims()[0];

  /** Iterate over all input images **/
  for (size_t image_id = 0; image_id < vreader.size(); ++image_id) {
    /** Iterate over all pixel in image (b,g,r) **/
    for (size_t pid = 0; pid < image_size; pid++) {
      /** Iterate over all channels **/
      for (size_t ch = 0; ch < num_chanels; ++ch) {
        /**          [images stride + channels stride + pixel id ] all in bytes
         * **/
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
  const InferenceEngine::TBlob<float>::Ptr fOutput =
      std::dynamic_pointer_cast<InferenceEngine::TBlob<float>>(
          output_blobs_[network_output_name_]);
  GetFeatures(*fOutput, features);
}
