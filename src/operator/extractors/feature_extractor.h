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

#ifndef SAF_OPERATOR_EXTRACTORS_FEATURE_EXTRACTOR_H_
#define SAF_OPERATOR_EXTRACTORS_FEATURE_EXTRACTOR_H_

#include "common/context.h"
#include "model/model.h"
#include "operator/operator.h"

class BaseFeatureExtractor {
 public:
  BaseFeatureExtractor() {}
  virtual ~BaseFeatureExtractor() {}
  virtual bool Init() = 0;
  virtual void Extract(const cv::Mat& image, const std::vector<Rect>& bboxes,
                       std::vector<std::vector<double>>& features) = 0;
};

class FeatureExtractor : public Operator {
 public:
  FeatureExtractor(const ModelDesc& model_desc, size_t batch_size,
                   const std::string& extractor_type);
  static std::shared_ptr<FeatureExtractor> Create(
      const FactoryParamsType& params);
  static std::string GetSourceName(int index) {
    return "input" + std::to_string(index);
  }
  static std::string GetSinkName(int index) {
    return "output" + std::to_string(index);
  }

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  ModelDesc model_desc_;
  size_t batch_size_;
  std::string extractor_type_;
  std::unique_ptr<BaseFeatureExtractor> extractor_;
};

#endif  // SAF_OPERATOR_EXTRACTORS_FEATURE_EXTRACTOR_H_
