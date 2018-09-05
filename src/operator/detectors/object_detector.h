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

// Multi-target detection using FRCNN

#ifndef SAF_OPERATOR_DETECTORS_OBJECT_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_OBJECT_DETECTOR_H_

#include <set>

#include "common/context.h"
#include "model/model.h"
#include "operator/operator.h"

struct ObjectInfo {
  ObjectInfo() { face_landmark_flag = false; }
  std::string tag;
  cv::Rect bbox;
  float confidence;
  FaceLandmark face_landmark;
  bool face_landmark_flag;
};

class BaseDetector {
 public:
  BaseDetector() {}
  virtual ~BaseDetector() {}
  virtual bool Init() = 0;
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image) = 0;
};

class ObjectDetector : public Operator {
 public:
  ObjectDetector(
      const std::string& type,
      const std::vector<ModelDesc>& model_descs = std::vector<ModelDesc>(),
      size_t batch_size = 1, float confidence_threshold = .5f,
      float idle_duration = 0.f,
      const std::set<std::string>& targets = std::set<std::string>(),
      int face_min_size = 40);
  static std::shared_ptr<ObjectDetector> Create(
      const FactoryParamsType& params);
  void SetInputStream(int src_id, StreamPtr stream);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string type_;
  std::vector<ModelDesc> model_descs_;
  size_t batch_size_;
  float confidence_threshold_;
  float idle_duration_;
  std::vector<std::chrono::time_point<std::chrono::system_clock>>
      last_detect_time_;
  std::set<std::string> targets_;
  int face_min_size_;
  std::unique_ptr<BaseDetector> detector_;
};

#endif  // SAF_OPERATOR_DETECTORS_OBJECT_DETECTOR_H_
