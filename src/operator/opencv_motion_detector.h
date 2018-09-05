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

// Motion detector using opencv

#ifndef SAF_OPERATOR_OPENCV_MOTION_DETECTOR_H_
#define SAF_OPERATOR_OPENCV_MOTION_DETECTOR_H_

#include <chrono>
#include <opencv2/opencv.hpp>
#include "operator.h"

class OpenCVMotionDetector : public Operator {
 public:
  OpenCVMotionDetector(float threshold = 0.5, float max_duration = 1.0);
  static std::shared_ptr<OpenCVMotionDetector> Create(
      const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  int GetPixels(cv::Mat& image);

 private:
  std::unique_ptr<cv::BackgroundSubtractorMOG2> mog2_;
  bool first_frame_;
  cv::Mat previous_fore_;
  int previous_pixels_;
  std::chrono::time_point<std::chrono::system_clock> last_send_time_;
  float threshold_;
  float max_duration_;
};

#endif  // SAF_OPERATOR_OPENCV_MOTION_DETECTOR_H_
