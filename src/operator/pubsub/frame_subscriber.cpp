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

#include "frame_subscriber.h"

#include <boost/archive/binary_iarchive.hpp>
#include <zguide/examples/C++/zhelpers.hpp>

constexpr auto SINK = "output";

FrameSubscriber::FrameSubscriber(const std::string url)
    : Operator(OPERATOR_TYPE_FRAME_SUBSCRIBER, {}, {SINK}),
      zmq_context_{1},
      zmq_subscriber_{zmq_context_, ZMQ_SUB} {
  // Bind the subscriber socket
  std::string zmq_subscriber_addr = "tcp://" + url;
  LOG(INFO) << "Subscribing to " << zmq_subscriber_addr;
  try {
    zmq_subscriber_.connect(zmq_subscriber_addr);
    zmq_subscriber_.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  } catch (const zmq::error_t& e) {
    LOG(FATAL) << "ZMQ connect error: " << e.what();
  }
}

FrameSubscriber::~FrameSubscriber() {
  // Clean up subscriber socket
  zmq_subscriber_.close();
}

StreamPtr FrameSubscriber::GetSink() { return Operator::GetSink(SINK); }

std::shared_ptr<FrameSubscriber> FrameSubscriber::Create(
    const FactoryParamsType& params) {
  std::string name = params.at("name");
  if (params.count("url") != 0) {
    return std::make_shared<FrameSubscriber>(params.at("url"));
  }
  return std::make_shared<FrameSubscriber>();
}

bool FrameSubscriber::Init() { return true; }

bool FrameSubscriber::OnStop() { return true; }

void FrameSubscriber::Process() {
  std::unique_ptr<Frame> frame;
  std::stringstream frame_string;

  // Initialize poll set
  zmq::pollitem_t items[] = {
      {static_cast<void*>(zmq_subscriber_), 0, ZMQ_POLLIN, 0}};

  // Check to see if there is a message waiting and receive it
  zmq::poll(items, 1, 0);
  if (items[0].revents & ZMQ_POLLIN) {
    frame_string << s_recv(zmq_subscriber_);
  } else {
    return;
  }

  try {
    boost::archive::binary_iarchive ar(frame_string);
    ar >> frame;
  } catch (const boost::archive::archive_exception& e) {
    LOG(INFO) << "Boost serialization error: " << e.what();
  }

  PushFrame(SINK, std::move(frame));
}
