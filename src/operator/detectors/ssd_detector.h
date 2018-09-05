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

#ifndef SAF_OPERATOR_DETECTORS_SSD_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_SSD_DETECTOR_H_

#include <set>

#include <caffe/caffe.hpp>

#include "model/model.h"
#include "operator/detectors/object_detector.h"

namespace ssd {
class Detector {
 public:
  Detector(const std::string& model_file, const std::string& weights_file,
           const std::string& mean_file, const std::string& mean_value);

  std::vector<std::vector<float> > Detect(const cv::Mat& img);

 private:
  void SetMean(const std::string& mean_file, const std::string& mean_value);

  void WrapInputLayer(std::vector<cv::Mat>* input_channels);

  void Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);

 private:
  std::shared_ptr<caffe::Net<float> > net_;
  cv::Size input_geometry_;
  size_t num_channels_;
  cv::Mat mean_;
};
}  // namespace ssd

class SsdDetector : public BaseDetector {
 public:
  SsdDetector(const ModelDesc& model_desc) : model_desc_(model_desc) {}
  virtual ~SsdDetector() {}
  virtual bool Init();
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image);

 private:
  std::string GetLabelName(int label) const;

 private:
  ModelDesc model_desc_;
  std::unique_ptr<ssd::Detector> detector_;
  caffe::LabelMap label_map_;
};

#endif  // SAF_OPERATOR_DETECTORS_SSD_DETECTOR_H_
