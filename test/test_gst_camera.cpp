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

#include <gtest/gtest.h>
#include "camera/gst_camera.h"

TEST(GST_CAMERA_TEST, TEST_BASIC) {
  std::string camera_name = "TEST_CAMERA";
  std::string video_uri =
      "gst://videotestsrc ! video/x-raw,width=640,height=480";
  int width = 640;
  int height = 480;
  GSTCamera camera(camera_name, video_uri, 640, 480);

  auto stream = camera.GetStream();
  auto reader = stream->Subscribe();
  camera.Start();

  const auto& image = reader->PopFrame()->GetValue<cv::Mat>("original_image");

  EXPECT_EQ(height, image.rows);
  EXPECT_EQ(width, image.cols);

  reader->UnSubscribe();
  camera.Stop();
}

TEST(GST_CAMERA_TEST, TEST_CAPTURE) {
  std::string camera_name = "TEST_CAMERA";
  std::string video_uri =
      "gst://videotestsrc ! video/x-raw,width=640,height=480";
  int width = 640;
  int height = 480;
  std::shared_ptr<Camera> camera(
      new GSTCamera(camera_name, video_uri, 640, 480));

  // Can capture image when camera is not started
  cv::Mat image;
  bool result = camera->Capture(image);
  EXPECT_EQ(true, result);
  EXPECT_EQ(height, image.rows);
  EXPECT_EQ(width, image.cols);

  // Can also capture image when camera is started
  camera->Start();
  result = camera->Capture(image);
  EXPECT_EQ(true, result);
  EXPECT_EQ(height, image.rows);
  EXPECT_EQ(width, image.cols);
  camera->Stop();
}
