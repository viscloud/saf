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

#ifndef SAF_OPERATOR_CVSDK_FEATURE_EXTRACTOR_H_
#define SAF_OPERATOR_CVSDK_FEATURE_EXTRACTOR_H_

#include <opencv2/opencv.hpp>
#include <unordered_map>

#include <ie_extension.h>
#include <ie_iexecutable_network.hpp>
#include <ie_plugin_ptr.hpp>
#include <inference_engine.hpp>

#include "cvsdk/cvsdk_base.h"
#include "operator/extractors/feature_extractor.h"

class CVSDKCNNFeatureExtractor : public BaseFeatureExtractor, public CVSDKBase {
 public:
  CVSDKCNNFeatureExtractor(const ModelDesc& model_desc)
      : model_desc_(model_desc) {}
  virtual ~CVSDKCNNFeatureExtractor() {}
  virtual bool Init();
  virtual void Extract(const cv::Mat& image, const std::vector<Rect>& bboxes,
                       std::vector<std::vector<double>>& features);
  virtual void ExtractBatch(const cv::Mat& image,
                            const std::vector<Rect>& bboxes,
                            std::vector<std::vector<double>>& features);

 private:
  ModelDesc model_desc_;
};

#endif  // SAF_OPERATOR_CVSDK_FEATURE_EXTRACTOR_H_
