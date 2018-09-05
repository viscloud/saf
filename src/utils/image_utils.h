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

#ifndef SAF_UTILS_IMAGE_UTILS_H_
#define SAF_UTILS_IMAGE_UTILS_H_

/**
 * @brief Rotate an OpenCV image matrix
 * @param m The OpenCV matrix
 * @param angle The angle to rotate; must be 0, 90, 180, or 270
 */
inline void RotateImage(cv::Mat& m, const int angle) {
  CHECK(angle == 0 || angle == 90 || angle == 180 || angle == 270)
      << "; angle was " << angle;

  if (angle == 90) {
    cv::transpose(m, m);
    cv::flip(m, m, 1);
  } else if (angle == 180) {
    cv::flip(m, m, -1);
  } else if (angle == 270) {
    cv::transpose(m, m);
    cv::flip(m, m, 0);
  }
}

#endif  // SAF_UTILS_IMAGE_UTILS_H_
