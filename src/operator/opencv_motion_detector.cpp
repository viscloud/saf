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

#include "operator/opencv_motion_detector.h"

OpenCVMotionDetector::OpenCVMotionDetector(float threshold, float max_duration)
    : Operator(OPERATOR_TYPE_OPENCV_MOTION_DETECTOR, {"input"}, {"output"}),
      first_frame_(true),
      previous_pixels_(0),
      threshold_(threshold),
      max_duration_(max_duration) {}

std::shared_ptr<OpenCVMotionDetector> OpenCVMotionDetector::Create(
    const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

bool OpenCVMotionDetector::Init() {
  mog2_.reset(cv::createBackgroundSubtractorMOG2());
  return true;
}

bool OpenCVMotionDetector::OnStop() {
  mog2_ = nullptr;
  return true;
}

void OpenCVMotionDetector::Process() {
  auto frame = GetFrame("input");
  auto image = frame->GetValue<cv::Mat>("image");

  cv::Mat fore;
  mog2_->apply(image, fore);

  cv::erode(fore, fore, cv::Mat());
  cv::dilate(fore, fore, cv::Mat());
  cv::erode(fore, fore, cv::Mat());
  cv::dilate(fore, fore, cv::Mat());
  cv::erode(fore, fore, cv::Mat());
  cv::dilate(fore, fore, cv::Mat());

  cv::Mat frameDelta;
  bool need_send = false;
  if (first_frame_) {
    first_frame_ = false;
    need_send = true;
  } else {
    cv::absdiff(fore, previous_fore_, frameDelta);
    int diff_pixels = GetPixels(frameDelta);
    if (diff_pixels >= (previous_pixels_ * threshold_)) {
      need_send = true;
    }
  }
  previous_fore_ = fore;
  previous_pixels_ = GetPixels(fore);

  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = now - last_send_time_;
  if (need_send || (diff.count() >= max_duration_)) {
    last_send_time_ = now;
    PushFrame("output", std::move(frame));
  }
}

int OpenCVMotionDetector::GetPixels(cv::Mat& image) {
  int pixels = 0;
  int nr = image.rows;
  int nc = image.cols * image.channels();
  for (int j = 0; j < nr; j++) {
    uchar* data = image.ptr<uchar>(j);
    for (int i = 0; i < nc; i++) {
      if (data[i]) pixels++;
    }
  }
  return pixels;
}
