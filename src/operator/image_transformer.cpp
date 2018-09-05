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

#include "operator/image_transformer.h"

#include "model/model_manager.h"
#include "utils/image_utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

const char* ImageTransformer::kOutputKey = "image";

ImageTransformer::ImageTransformer(const Shape& target_shape, bool crop,
                                   unsigned int angle)
    : Operator(OPERATOR_TYPE_IMAGE_TRANSFORMER, {SOURCE_NAME}, {SINK_NAME}),
      target_shape_(target_shape),
      crop_(crop),
      angle_(angle) {}

std::shared_ptr<ImageTransformer> ImageTransformer::Create(
    const FactoryParamsType& params) {
  int width = atoi(params.at("width").c_str());
  int height = atoi(params.at("height").c_str());

  // Default channel = 3
  int num_channels = 3;
  if (params.count("channels") != 0) {
    num_channels = atoi(params.at("channels").c_str());
  }
  CHECK(width >= 0 && height >= 0 && num_channels >= 0)
      << "Width (" << width << "), height (" << height
      << "), and number of channels (" << num_channels
      << ") must not be negative.";

  return std::make_shared<ImageTransformer>(Shape(num_channels, width, height),
                                            true);
}

void ImageTransformer::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr ImageTransformer::GetSink() { return Operator::GetSink(SINK_NAME); }

void ImageTransformer::Process() {
  auto frame = GetFrame(SOURCE_NAME);
  const cv::Mat& img = frame->GetValue<cv::Mat>("original_image");

  int num_channel = target_shape_.channel;
  int width = target_shape_.width;
  int height = target_shape_.height;
  cv::Size input_geometry(width, height);

  cv::Mat sample_image;
  // Convert channels
  if (img.channels() == 3 && num_channel == 1)
    cv::cvtColor(img, sample_image, cv::COLOR_BGR2GRAY);
  else if (img.channels() == 4 && num_channel == 1)
    cv::cvtColor(img, sample_image, cv::COLOR_BGRA2GRAY);
  else if (img.channels() == 4 && num_channel == 3)
    cv::cvtColor(img, sample_image, cv::COLOR_BGRA2BGR);
  else if (img.channels() == 1 && num_channel == 3)
    cv::cvtColor(img, sample_image, cv::COLOR_GRAY2BGR);
  else
    sample_image = img;

  cv::Mat sample_cropped;
  // Crop according to scale
  if (crop_) {
    int desired_width = (int)((float)width / height * img.size[1]);
    int desired_height = (int)((float)height / width * img.size[0]);
    int new_width = img.size[0];
    int new_height = img.size[1];
    if (desired_width < img.size[0]) {
      new_width = desired_width;
    } else {
      new_height = desired_height;
    }
    cv::Rect roi((img.size[1] - new_height) / 2, (img.size[0] - new_width) / 2,
                 new_width, new_height);
    sample_cropped = sample_image(roi);
  } else {
    sample_cropped = sample_image;
  }

  // Resize
  cv::Mat sample_resized;
  if (sample_cropped.size() != input_geometry) {
    cv::resize(sample_cropped, sample_resized, input_geometry);
  } else {
    sample_resized = sample_cropped;
  }

  // Rotate
  if (angle_ != 0) {
    RotateImage(sample_resized, angle_);
  }

  frame->SetValue("image", sample_resized);
  PushFrame(SINK_NAME, std::move(frame));
}

bool ImageTransformer::Init() { return true; }
bool ImageTransformer::OnStop() { return true; }
