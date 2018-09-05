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

#include "operator/opencv_optical_flow.h"

OpenCVOpticalFlow::OpenCVOpticalFlow()
    : Operator(OPERATOR_TYPE_OPENCV_OPTICAL_FLOW, {"input"}, {"output"}) {}

std::shared_ptr<OpenCVOpticalFlow> OpenCVOpticalFlow::Create(
    const FactoryParamsType&) {
  return std::make_shared<OpenCVOpticalFlow>();
}

bool OpenCVOpticalFlow::Init() { return true; }

bool OpenCVOpticalFlow::OnStop() { return true; }

void OpenCVOpticalFlow::Process() {
  auto frame = GetFrame("input");
  auto image = frame->GetValue<cv::Mat>("original_image");

  cv::Mat flow, cflow;
  cv::UMat gray, uflow;
  cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
  if (!prevgray_.empty()) {
    calcOpticalFlowFarneback(prevgray_, gray, uflow, 0.5, 3, 15, 3, 5, 1.2, 0);
    cvtColor(prevgray_, cflow, cv::COLOR_GRAY2BGR);
    uflow.copyTo(flow);

    frame->SetValue("cflow", cflow);
    frame->SetValue("flow", flow);
    PushFrame("output", std::move(frame));
  }

  std::swap(prevgray_, gray);
}
