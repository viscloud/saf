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

#include "operator/flow_control/flow_control_exit.h"

#include "model/model_manager.h"
#include "operator/flow_control/flow_control_entrance.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

FlowControlExit::FlowControlExit()
    : Operator(OPERATOR_TYPE_FLOW_CONTROL_EXIT, {SOURCE_NAME}, {SINK_NAME}) {}

std::shared_ptr<FlowControlExit> FlowControlExit::Create(
    const FactoryParamsType&) {
  return std::make_shared<FlowControlExit>();
}

void FlowControlExit::SetSink(StreamPtr stream) {
  Operator::SetSink(SINK_NAME, stream);
}

void FlowControlExit::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr FlowControlExit::GetSink() { return Operator::GetSink(SINK_NAME); }

bool FlowControlExit::Init() { return true; }

bool FlowControlExit::OnStop() { return true; }

void FlowControlExit::Process() {
  auto frame = GetFrame(SOURCE_NAME);
  auto entrance = frame->GetFlowControlEntrance();
  if (entrance) {
    entrance->ReturnToken(frame->GetValue<unsigned long>("frame_id"));
    // Change the frame's FlowControlEntrance to null so that it does not try to
    // release the token again.
    frame->SetFlowControlEntrance(nullptr);
  }
  PushFrame(SINK_NAME, std::move(frame));
}
