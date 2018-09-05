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

#ifndef SAF_OPERATOR_DETECTORS_OPENCV_PEOPLE_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_OPENCV_PEOPLE_DETECTOR_H_

#ifdef USE_CUDA
#include <opencv2/cudaobjdetect.hpp>
#else
#include <opencv2/objdetect/objdetect.hpp>
#endif  // USE_CUDA

#include "operator/detectors/object_detector.h"
#include "operator/operator.h"

class OpenCVPeopleDetector : public BaseDetector {
 public:
  OpenCVPeopleDetector() {}
  virtual ~OpenCVPeopleDetector() {}
  virtual bool Init();
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image);

 private:
  cv::HOGDescriptor hog_;
};

#endif  // SAF_OPERATOR_DETECTORS_OPENCV_PEOPLE_DETECTOR_H_
