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

// CV utils

#ifndef SAF_UTILS_CV_UTILS_H_
#define SAF_UTILS_CV_UTILS_H_

#include <opencv2/opencv.hpp>

inline cv::Scalar HSV2RGB(const float h, const float s, const float v) {
  const int h_i = static_cast<int>(h * 6);
  const float f = h * 6 - h_i;
  const float p = v * (1 - s);
  const float q = v * (1 - f * s);
  const float t = v * (1 - (1 - f) * s);
  float r, g, b;
  switch (h_i) {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    case 5:
      r = v;
      g = p;
      b = q;
      break;
    default:
      r = 1;
      g = 1;
      b = 1;
      break;
  }
  return cv::Scalar(r * 255, g * 255, b * 255);
}

// http://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically
inline std::vector<cv::Scalar> GetColors(const int n) {
  std::vector<cv::Scalar> colors;
  cv::RNG rng(12345);
  const float golden_ratio_conjugate = 0.618033988749895;
  const float s = 0.3;
  const float v = 0.99;
  for (int i = 0; i < n; ++i) {
    const float h =
        std::fmod(rng.uniform(0.f, 1.f) + golden_ratio_conjugate, 1.f);
    colors.push_back(HSV2RGB(h, s, v));
  }
  return colors;
}

inline cv::Mat FixupChannels(const cv::Mat& img, int num_channels) {
  cv::Mat result_img = img;

  // Convert the input image to the input image format of the network
  if (img.channels() != num_channels) {
    cv::Mat sample;
    if (img.channels() == 3 && num_channels == 1)
      cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
    else if (img.channels() == 4 && num_channels == 1)
      cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
    else if (img.channels() == 4 && num_channels == 3)
      cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    else if (img.channels() == 1 && num_channels == 3)
      cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    else {
      // Should not enter here, just in case.
      sample = img;
    }
    result_img = sample;
  }

  return result_img;
}

inline std::shared_ptr<std::vector<unsigned char>> OCVReaderGetData(
    const cv::Mat& img, int width = 0, int height = 0) {
  cv::Mat resized(img);
  if (width != 0 && height != 0) {
    cv::resize(img, resized, cv::Size(width, height));
  }

  size_t size =
      resized.size().width * resized.size().height * resized.channels();
  auto _data = std::make_shared<std::vector<unsigned char>>(size);
  for (size_t id = 0; id < size; ++id) {
    _data->at(id) = resized.data[id];
  }
  return _data;
}

inline bool InsideImage(const cv::Rect& rt, const cv::Mat& image) {
  int x = rt.x;
  int y = rt.y;
  int w = rt.width;
  int h = rt.height;
  return ((x >= 0) && (y >= 0) && (x + w <= image.cols) &&
          (y + h <= image.rows));
}

#endif  // SAF_UTILS_CV_UTILS_H_
