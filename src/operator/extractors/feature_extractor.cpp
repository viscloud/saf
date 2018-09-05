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

// Feature extractor

#include "operator/extractors/feature_extractor.h"

#include <cv.h>

#include "common/context.h"
#include "model/model_manager.h"
#ifdef HAVE_INTEL_CAFFE
#include "operator/extractors/caffe_feature_extractor.h"
#endif  // HAVE_INTEL_CAFFE
#ifdef USE_CVSDK
#include "operator/extractors/cvsdk_feature_extractor.h"
#endif  // USE_CVSDK
#include "utils/string_utils.h"

FeatureExtractor::FeatureExtractor(const ModelDesc& model_desc,
                                   size_t batch_size,
                                   const std::string& extractor_type)
    : Operator(OPERATOR_TYPE_FEATURE_EXTRACTOR, {}, {}),
      model_desc_(model_desc),
      batch_size_(batch_size),
      extractor_type_(extractor_type) {
  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({GetSourceName(i), nullptr});
    sinks_.insert({GetSinkName(i), std::shared_ptr<Stream>(new Stream)});
  }

  LOG(INFO) << "batch size of " << batch_size_;
}

std::shared_ptr<FeatureExtractor> FeatureExtractor::Create(
    const FactoryParamsType& params) {
  ModelManager& model_manager = ModelManager::GetInstance();
  auto model_name = params.at("model");
  CHECK(model_manager.HasModel(model_name));
  auto model_desc = model_manager.GetModelDesc(model_name);

  auto batch_size = StringToSizet(params.at("batch_size"));
  auto extractor_type = params.at("extractor_type");

  return std::make_shared<FeatureExtractor>(model_desc, batch_size,
                                            extractor_type);
}

bool FeatureExtractor::Init() {
  bool result = false;
#ifdef HAVE_INTEL_CAFFE
  if (extractor_type_ == "caffe-cnn") {
    extractor_ = std::make_unique<CaffeCNNFeatureExtractor>(model_desc_);
    result = extractor_->Init();
  } else
#endif  // HAVE_INTEL_CAFFE
#ifdef USE_CVSDK
      if (extractor_type_ == "cvsdk-cnn") {
    extractor_.reset(new CVSDKCNNFeatureExtractor(model_desc_));
    result = extractor_->Init();
  } else
#endif  // USE_CVSDK
  {
    LOG(FATAL) << "Extractor type " << extractor_type_ << " not supported.";
  }
  return result;
}

bool FeatureExtractor::OnStop() { return true; }

void FeatureExtractor::Process() {
  Timer timer;
  timer.Start();

  for (size_t i = 0; i < batch_size_; i++) {
    auto frame = GetFrame(GetSourceName(i));
    if (!frame) continue;

    std::vector<std::vector<double>> features;
    auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    if (bboxes.size() > 0) {
      cv::Mat image = frame->GetValue<cv::Mat>("original_image");
      extractor_->Extract(image, bboxes, features);
    }

    frame->SetValue("features", features);
    PushFrame(GetSinkName(i), std::move(frame));
  }

  LOG(INFO) << "FeatureExtractor took " << timer.ElapsedMSec() << " ms";
}
