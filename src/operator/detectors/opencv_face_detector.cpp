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

#include "operator/detectors/opencv_face_detector.h"

bool OpenCVFaceDetector::Init() {
  return classifier_.load(model_desc_.GetModelParamsPath());
}

std::vector<ObjectInfo> OpenCVFaceDetector::Detect(const cv::Mat& image) {
  std::vector<ObjectInfo> result;

  std::vector<cv::Rect> rects;
  classifier_.detectMultiScale(image, rects);
  for (const auto& m : rects) {
    ObjectInfo object_info;
    object_info.tag = "face";
    object_info.bbox = m;
    object_info.confidence = 1.0;
    result.push_back(object_info);
  }

  return result;
}
