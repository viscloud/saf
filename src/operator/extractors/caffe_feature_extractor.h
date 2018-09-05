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

// Feature extractor using caffe

#ifndef SAF_OPERATOR_EXTRACTORS_CAFFE_FEATURE_EXTRACTOR_H_
#define SAF_OPERATOR_EXTRACTORS_CAFFE_FEATURE_EXTRACTOR_H_

#include <caffe/caffe.hpp>
#include <caffe/data_transformer.hpp>

#include "operator/extractors/feature_extractor.h"

class CaffeCNNFeatureExtractor : public BaseFeatureExtractor {
 public:
  CaffeCNNFeatureExtractor(const ModelDesc& model_desc)
      : model_desc_(model_desc) {}
  virtual ~CaffeCNNFeatureExtractor() {}
  virtual bool Init();
  virtual void Extract(const cv::Mat& image, const std::vector<Rect>& bboxes,
                       std::vector<std::vector<double>>& features);

 private:
  std::unique_ptr<caffe::Net<float>> net_;
  ModelDesc model_desc_;
  int num_channels_;
  cv::Mat mean_;
  cv::Size input_blob_size_;
  std::unique_ptr<caffe::DataTransformer<float>> data_transformer_;
};

#endif  // SAF_OPERATOR_EXTRACTORS_CAFFE_FEATURE_EXTRACTOR_H_
