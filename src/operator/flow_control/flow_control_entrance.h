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

#ifndef SAF_OPERATOR_FLOW_CONTROL_FLOW_CONTROL_ENTRANCE_H_
#define SAF_OPERATOR_FLOW_CONTROL_FLOW_CONTROL_ENTRANCE_H_

#include <memory>
#include <mutex>

#include "common/types.h"
#include "operator/operator.h"

// FlowControlEntrance performs admission control of frames to limit the number
// of outstanding frames in the pipeline. It should be used together with
// FlowControlExit.
class FlowControlEntrance : public Operator {
 public:
  // "max_tokens" should not be larger than the capacity of the shortest stream
  // queue in the flow control domain. This is to ensure that no frames are
  // dropped due to queue overflow.
  FlowControlEntrance(unsigned int max_tokens);
  void ReturnToken(unsigned long frame_id);
  static std::shared_ptr<FlowControlEntrance> Create(
      const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  // Used to verify that num_tokens_available_ never exceeds the original number
  // of tokens.
  unsigned int max_tokens_;
  unsigned int num_tokens_available_;
  // IDs of frame with tokens
  std::unordered_set<unsigned long> frames_with_tokens_;
  std::mutex mtx_;
};

#endif  // SAF_OPERATOR_FLOW_CONTROL_FLOW_CONTROL_ENTRANCE_H_
