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

// Multi-target detection using mobilenet ssd

#ifndef SAF_OPERATOR_DETECTORS_CAFFE_MOBILENET_SSD_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_CAFFE_MOBILENET_SSD_DETECTOR_H_

#include <caffe/caffe.hpp>
#include <caffe/data_transformer.hpp>

#include "common/context.h"
#include "model/model.h"
#include "operator/detectors/object_detector.h"

template <typename Dtype>
class MobilenetSsdDetector : public BaseDetector {
 public:
  MobilenetSsdDetector(const ModelDesc& model_desc) : model_desc_(model_desc) {}
  virtual ~MobilenetSsdDetector() {}
  virtual bool Init();
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image);

 private:
  ModelDesc model_desc_;
  std::vector<std::string> voc_names_;
  std::unique_ptr<caffe::Net<Dtype>> net_;
  cv::Size input_geometry_;
  int num_channels_;
  cv::Mat mean_;
  cv::Size input_blob_size_;
  std::unique_ptr<caffe::DataTransformer<Dtype>> data_transformer_;
};

#endif  // SAF_OPERATOR_DETECTORS_CAFFE_MOBILENET_SSD_DETECTOR_H_
