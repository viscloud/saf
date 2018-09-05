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

#include "operator/buffer.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

Buffer::Buffer(unsigned long num_frames)
    : Operator(OPERATOR_TYPE_BUFFER, {SOURCE_NAME}, {SINK_NAME}),
      buffer_{num_frames} {}

std::shared_ptr<Buffer> Buffer::Create(const FactoryParamsType& params) {
  return std::make_shared<Buffer>(std::stoi(params.at("num_frames")));
}

void Buffer::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr Buffer::GetSink() { return Operator::GetSink(SINK_NAME); }

bool Buffer::Init() { return true; }

bool Buffer::OnStop() { return true; }

void Buffer::Process() {
  if (buffer_.full()) {
    PushFrame(SINK_NAME, std::move(buffer_.front()));
    buffer_.pop_front();
  }

  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);
  buffer_.push_back(std::move(frame));
}
