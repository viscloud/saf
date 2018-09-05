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

// Optical flow using opencv

#ifndef SAF_OPERATOR_OPENCV_OPTICAL_FLOW_H_
#define SAF_OPERATOR_OPENCV_OPTICAL_FLOW_H_

#include <opencv2/opencv.hpp>

#include "operator.h"

class OpenCVOpticalFlow : public Operator {
 public:
  OpenCVOpticalFlow();
  static std::shared_ptr<OpenCVOpticalFlow> Create(
      const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  cv::UMat prevgray_;
};

#endif  // SAF_OPERATOR_OPENCV_OPTICAL_FLOW_H_
