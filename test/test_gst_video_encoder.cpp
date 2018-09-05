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

#include <memory>

#include <gtest/gtest.h>

#include "camera/camera_manager.h"
#include "stream/frame.h"
#include "utils/utils.h"
#include "video/gst_video_encoder.h"

TEST(TestGstVideoEncoder, TestFile) {
  auto camera = CameraManager::GetInstance().GetCamera("GST_TEST");
  auto encoder =
      std::make_shared<GstVideoEncoder>("original_image", "/tmp/test.mp4");
  encoder->SetSource(camera->GetStream());

  auto encoder_reader = encoder->GetSink()->Subscribe();
  camera->Start();
  encoder->Start();

  auto image_frame = encoder_reader->PopFrame();

  encoder_reader->UnSubscribe();
  encoder->Stop();
  camera->Stop();

  std::remove("test.mp4");
}

TEST(TestGstVideoEncoder, TestStream) {
  auto camera = CameraManager::GetInstance().GetCamera("GST_TEST");
  auto encoder = std::make_shared<GstVideoEncoder>("original_image", "", 12345);
  encoder->SetSource(camera->GetStream());

  camera->Start();
  encoder->Start();

  SAF_SLEEP(100);

  encoder->Stop();
  camera->Stop();
}
