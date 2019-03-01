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

#include "operator/flow_control/flow_control_entrance.h"

#include <stdexcept>

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

FlowControlEntrance::FlowControlEntrance(unsigned int max_tokens, bool block)
    : Operator(OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE, {SOURCE_NAME}, {SINK_NAME}),
      max_tokens_(max_tokens),
      num_tokens_available_(max_tokens),
      block_(block) {}

std::shared_ptr<FlowControlEntrance> FlowControlEntrance::Create(
    const FactoryParamsType& params) {
  int max_tokens = std::stoi(params.at("max_tokens"));
  if (max_tokens < 0) {
    throw std::invalid_argument("\"max_tokens\" cannot be negative, but is: " +
                                std::to_string(max_tokens));
  }
  return std::make_shared<FlowControlEntrance>((unsigned int)max_tokens);
}

void FlowControlEntrance::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr FlowControlEntrance::GetSink() {
  return Operator::GetSink(SINK_NAME);
}

bool FlowControlEntrance::Init() { return true; }

bool FlowControlEntrance::OnStop() {
  std::unique_lock<std::mutex> lock(mtx_);
  block_cv_.notify_one();
  return true;
}

void FlowControlEntrance::Process() {
  auto frame = GetFrame(SOURCE_NAME);
  unsigned long id = frame->GetValue<unsigned long>("frame_id");
  if (frame->GetFlowControlEntrance()) {
    throw std::runtime_error("Frame " + std::to_string(id) +
                             " is already under flow control.");
  }

  // Used to minimize the length of the critical section.
  bool push = false;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    if (block_) {
      // Wait until a token is available.
      block_cv_.wait(lock,
                     [this] { return stopped_ || num_tokens_available_ > 0; });
      if (stopped_) {
        return;
      }
    }
    if (num_tokens_available_) {
      frames_with_tokens_.insert(id);
      --num_tokens_available_;
      push = true;
    } else if (block_) {
      LOG(ERROR) << "FlowControlEntrance attempting to drop a frame while in "
                    "\"block\" mode!";
    }
  }

  if (push) {
    frame->SetFlowControlEntrance(this);
    PushFrame(SINK_NAME, std::move(frame));
  } else {
    LOG(WARNING) << "Insufficient flow control tokens. Dropping frame: " << id;
  }
}

void FlowControlEntrance::ReturnToken(unsigned long frame_id) {
  std::unique_lock<std::mutex> lock(mtx_);
  if (frames_with_tokens_.find(frame_id) == frames_with_tokens_.end()) {
    LOG(INFO) << "Frame " << frame_id
              << " releasing token that was not issued.";
  } else {
    frames_with_tokens_.erase(frame_id);

    ++num_tokens_available_;

    if (block_) {
      // If we are in blocking mode, then wake up the Process() function.
      block_cv_.notify_one();
    }

    if (num_tokens_available_ > max_tokens_) {
      throw std::runtime_error(
          "More flow control tokens have been returned than were distributed.");
    }
  }
}
