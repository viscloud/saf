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

#ifndef SAF_OPERATOR_PUBSUB_FRAME_PUBLISHER_H_
#define SAF_OPERATOR_PUBSUB_FRAME_PUBLISHER_H_

#include <string>
#include <unordered_set>

#include <zmq.hpp>

#include "common/types.h"
#include "operator/operator.h"

static const std::string DEFAULT_ZMQ_PUB_URL = "127.0.0.1:5536";

/**
 * @brief A class for publishing a stream on the network using ZMQ.
 */
class FramePublisher : public Operator {
 public:
  // "fields_to_send" specifies which fframe fields to publish. The empty set
  // implies all fields.
  FramePublisher(const std::string& url = DEFAULT_ZMQ_PUB_URL,
                 std::unordered_set<std::string> fields_to_send = {});
  ~FramePublisher();

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  static std::shared_ptr<FramePublisher> Create(
      const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  zmq::context_t zmq_context_;
  zmq::socket_t zmq_publisher_;
  std::string zmq_publisher_addr_;
  // The frame fields to send. The empty set implies all fields.
  std::unordered_set<std::string> fields_to_send_;
};

#endif  // SAF_OPERATOR_PUBSUB_FRAME_PUBLISHER_H_
