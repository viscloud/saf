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
#include "stream/stream.h"

TEST(STREAM_TEST, BASIC_TEST) {
  std::shared_ptr<Stream> stream(new Stream);
  auto reader = stream->Subscribe();

  auto input_frame = std::make_unique<Frame>();
  input_frame->SetValue("image", cv::Mat(10, 20, CV_8UC3));
  stream->PushFrame(std::move(input_frame));

  auto output_frame = reader->PopFrame();
  const auto& image = output_frame->GetValue<cv::Mat>("image");
  EXPECT_EQ(image.rows, 10);
  EXPECT_EQ(image.cols, 20);

  reader->UnSubscribe();
}

TEST(STREAM_TEST, SUBSCRIBE_TEST) {
  std::shared_ptr<Stream> stream(new Stream);
  auto reader1 = stream->Subscribe();
  auto reader2 = stream->Subscribe();

  stream->PushFrame(std::make_unique<Frame>());
  stream->PushFrame(std::make_unique<Frame>());

  // Both readers can pop twice
  reader1->PopFrame();
  reader1->PopFrame();

  reader2->PopFrame();
  reader2->PopFrame();

  reader1->UnSubscribe();
  reader2->UnSubscribe();
}
