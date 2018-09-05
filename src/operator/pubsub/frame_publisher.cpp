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

#include "operator/pubsub/frame_publisher.h"

#include <boost/archive/binary_oarchive.hpp>
#include <zguide/examples/C++/zhelpers.hpp>

constexpr auto SOURCE = "input";

FramePublisher::FramePublisher(const std::string& url,
                               std::unordered_set<std::string> fields_to_send)
    : Operator(OPERATOR_TYPE_FRAME_PUBLISHER, {SOURCE}, {}),
      zmq_context_{1},
      zmq_publisher_{zmq_context_, ZMQ_PUB},
      zmq_publisher_addr_("tcp://" + url),
      fields_to_send_(fields_to_send) {
  // Bind the publisher socket
  LOG(INFO) << "Publishing frames on " << zmq_publisher_addr_;
  try {
    zmq_publisher_.bind(zmq_publisher_addr_);
  } catch (const zmq::error_t& e) {
    LOG(FATAL) << "ZMQ bind error: " << e.what();
  }
}

FramePublisher::~FramePublisher() {
  // Tear down the publisher socket
  zmq_publisher_.unbind(zmq_publisher_addr_);
}

void FramePublisher::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE, stream);
}

std::shared_ptr<FramePublisher> FramePublisher::Create(
    const FactoryParamsType& params) {
  if (params.count("url") != 0) {
    return std::make_shared<FramePublisher>(params.at("url"));
  }
  return std::make_shared<FramePublisher>();
}

bool FramePublisher::Init() { return true; }

bool FramePublisher::OnStop() { return true; }

void FramePublisher::Process() {
  auto frame = this->GetFrame(SOURCE);

  // serialize
  std::stringstream frame_string;
  try {
    boost::archive::binary_oarchive ar(frame_string);
    // Make a copy of the frame, keeping only the fields that we are supposed to
    // send.
    auto frame_to_send = std::make_unique<Frame>(frame, fields_to_send_);
    ar << frame_to_send;
  } catch (const boost::archive::archive_exception& e) {
    LOG(INFO) << "Boost serialization error: " << e.what();
  }

  s_send(zmq_publisher_, frame_string.str());
}
