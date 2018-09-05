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

#ifndef SAF_OPERATOR_PUBSUB_FRAME_SUBSCRIBER_H_
#define SAF_OPERATOR_PUBSUB_FRAME_SUBSCRIBER_H_

#include <zmq.hpp>

#include "common/types.h"
#include "operator/operator.h"

static const std::string DEFAULT_ZMQ_SUB_URL = "127.0.0.1:5536";

/**
 * @brief A class for subscribing to a stream using ZMQ.
 */
class FrameSubscriber : public Operator {
 public:
  FrameSubscriber(const std::string url = DEFAULT_ZMQ_SUB_URL);
  ~FrameSubscriber();

  StreamPtr GetSink();
  StreamPtr GetSink(const std::string& name) = delete;

  static std::shared_ptr<FrameSubscriber> Create(
      const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  zmq::context_t zmq_context_;
  zmq::socket_t zmq_subscriber_;
};

#endif  // SAF_OPERATOR_PUBSUB_FRAME_SUBSCRIBER_H_
