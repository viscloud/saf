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

#include "operator/strider.h"

#include "operator/flow_control/flow_control_entrance.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

Strider::Strider(unsigned long stride)
    : Operator(OPERATOR_TYPE_STRIDER, {SOURCE_NAME}, {SINK_NAME}),
      stride_(stride),
      num_frames_processed_(0) {}

std::shared_ptr<Strider> Strider::Create(const FactoryParamsType& params) {
  unsigned long stride = std::stoul(params.at("stride"));
  return std::make_shared<Strider>(stride);
}

void Strider::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr Strider::GetSink() { return Operator::GetSink(SINK_NAME); }

bool Strider::Init() { return true; }

bool Strider::OnStop() { return true; }

void Strider::Process() {
  auto frame = GetFrame(SOURCE_NAME);

  if (num_frames_processed_ % stride_) {
    // Drop frames whose arrival index is not evenly divisible by the stride.
    LOG(WARNING) << "Striding by " << stride_ << " frames. Dropping frame: "
                 << frame->GetValue<unsigned long>("frame_id");
    auto flow_control_entrance = frame->GetFlowControlEntrance();
    if (flow_control_entrance) {
      // If a flow control entrance exists, then we need to inform it that a
      // frame is being dropped so that the flow control token is returned.
      flow_control_entrance->ReturnToken(
          frame->GetValue<unsigned long>("frame_id"));
      // Change the frame's FlowControlEntrance to null so that it does not try
      // to release the token again.
      frame->SetFlowControlEntrance(nullptr);
    }
  } else {
    PushFrame(SINK_NAME, std::move(frame));
  }

  ++num_frames_processed_;
}
