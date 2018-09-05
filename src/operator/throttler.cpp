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

#include "operator/throttler.h"

#include "model/model_manager.h"
#include "operator/flow_control/flow_control_entrance.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

Throttler::Throttler(double fps)
    : Operator(OPERATOR_TYPE_THROTTLER, {SOURCE_NAME}, {SINK_NAME}),
      delay_ms_(0) {
  SetFps(fps);
}

std::shared_ptr<Throttler> Throttler::Create(const FactoryParamsType& params) {
  double fps = std::stod(params.at("fps"));
  return std::make_shared<Throttler>(fps);
}

void Throttler::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr Throttler::GetSink() { return Operator::GetSink(SINK_NAME); }

void Throttler::SetFps(double fps) {
  if (fps < 0) {
    throw std::invalid_argument("Fps cannot be negative!");
  } else if (fps == 0) {
    // Turn throttling off.
    delay_ms_ = 0;
  } else {
    delay_ms_ = 1000 / fps;
  }
}

bool Throttler::Init() { return true; }

bool Throttler::OnStop() { return true; }

void Throttler::Process() {
  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);

  if (timer_.ElapsedMSec() < delay_ms_) {
    // Drop frame.
    LOG(INFO) << "Frame rate too high. Dropping frame: "
              << frame->GetValue<unsigned long>("frame_id");

    FlowControlEntrance* flow_control_entrance =
        frame->GetFlowControlEntrance();
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
    // Restart timer
    timer_.Start();
    PushFrame(SINK_NAME, std::move(frame));
  }
}
