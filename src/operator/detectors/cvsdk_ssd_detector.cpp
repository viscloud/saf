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

// Multi-target detection using SSD

#include "operator/detectors/cvsdk_ssd_detector.h"

#include <ie_plugin_config.hpp>
#include <vpu/vpu_plugin_config.hpp>

#include <ie_extension.h>
#include "cvsdk/common.hpp"
#include "cvsdk/ext_list.hpp"

#include "common/context.h"
#include "model/model_manager.h"
#include "utils/cv_utils.h"
#include "utils/yolo_utils.h"

using namespace InferenceEngine::details;
using namespace InferenceEngine;

bool CVSDKSsdDetector::Init() {
  std::string labelmap_file = model_desc_.GetLabelFilePath();
  voc_names_ = ReadVocNames(labelmap_file);

  Initialize(model_desc_);

  OutputsDataMap outputsInfo(network_builder_.getNetwork().getOutputsInfo());
  auto firstOutputInfo = outputsInfo.begin()->second;
  SizeVector outputDims = firstOutputInfo->dims;
  Layout outputLayout = firstOutputInfo->layout;

  if (outputDims.size() != 4) {
    throw std::logic_error("Incorrect output dimensions for SSD model");
  }

  if (outputDims[0] != 7) {
    throw std::logic_error("Output item should have 7 as a last dimension");
  }

  LOG(INFO) << "CVSDKSsdDetector initialized";
  return true;
}

std::vector<ObjectInfo> CVSDKSsdDetector::Detect(const cv::Mat& image) {
  InputsDataMap inputsInfo(network_builder_.getNetwork().getInputsInfo());
  auto firstInputInfo = inputsInfo.begin()->second;
  std::shared_ptr<std::vector<unsigned char>> image_data(OCVReaderGetData(
      image, firstInputInfo->getDims()[0], firstInputInfo->getDims()[1]));
  if (image_data.get() == nullptr) {
    throw std::logic_error("Valid input images were not found!");
  }

  // Convert image into the OpenVINO format
  size_t num_chanels = input_->dims()[2];
  size_t image_size = input_->dims()[1] * input_->dims()[0];
  for (size_t pid = 0; pid < image_size; pid++) {
    for (size_t ch = 0; ch < num_chanels; ++ch) {
      input_->data()[ch * image_size + pid] =
          (*image_data)[pid * num_chanels + ch];
    }
  }

  OutputsDataMap outputsInfo(network_builder_.getNetwork().getOutputsInfo());
  SizeVector outputDims = outputsInfo.begin()->second->dims;
  Layout outputLayout = outputsInfo.begin()->second->layout;

  const int maxProposalCount = outputDims[1];
  const int objectSize = outputDims[0];

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
  const InferenceEngine::TBlob<float>::Ptr detectionOutArray =
      std::dynamic_pointer_cast<InferenceEngine::TBlob<float>>(
          output_blobs_[network_output_name_]);

  // Interperet bounding boxes from the output blob
  float* box = detectionOutArray->data();
  std::vector<ObjectInfo> result_object;
  for (int i = 0; i < maxProposalCount; i++) {
    ObjectInfo object_info;
    float image_id = box[i * objectSize + 0];
    float label = box[i * objectSize + 1];
    int classid = static_cast<int>(label);
    if (classid >= 0 && classid < (int)voc_names_.size())
      object_info.tag = voc_names_.at(classid);
    float confidence = box[i * objectSize + 2];
    float xmin = box[i * objectSize + 3] * image.cols;
    float ymin = box[i * objectSize + 4] * image.rows;
    float xmax = box[i * objectSize + 5] * image.cols;
    float ymax = box[i * objectSize + 6] * image.rows;

    if (image_id < 0 /* better than check == -1 */) {
      LOG(INFO) << "Only " << i << " proposals found" << std::endl;
      break;
    }

    cv::Point left_top(xmin, ymin);
    cv::Point right_bottom(xmax, ymax);
    object_info.bbox = cv::Rect(left_top, right_bottom);
    object_info.confidence = confidence;
    result_object.push_back(object_info);
  }

  return result_object;
}
