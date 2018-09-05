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

#include "frame_sender.h"

#include <boost/archive/binary_oarchive.hpp>

constexpr auto SOURCE = "input";

FrameSender::FrameSender(const std::string server_url)
    : Operator(OPERATOR_TYPE_FRAME_SENDER, {SOURCE}, {}),
      server_url_(server_url) {
  auto channel =
      grpc::CreateChannel(server_url_, grpc::InsecureChannelCredentials());
  stub_ = Messenger::NewStub(channel);
}

void FrameSender::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE, stream);
}

std::shared_ptr<FrameSender> FrameSender::Create(
    const FactoryParamsType& params) {
  return std::make_shared<FrameSender>(params.at("server_url"));
}

bool FrameSender::Init() { return true; }

bool FrameSender::OnStop() { return true; }

void FrameSender::Process() {
  auto frame = this->GetFrame(SOURCE);

  // serialize
  std::stringstream frame_string;
  try {
    boost::archive::binary_oarchive ar(frame_string);
    ar << frame;
  } catch (const boost::archive::archive_exception& e) {
    LOG(INFO) << "Boost serialization error: " << e.what();
  }

  SingleFrame frame_message;
  frame_message.set_frame(frame_string.str());

  grpc::ClientContext context;
  google::protobuf::Empty ignored;
  grpc::Status status = stub_->SendFrame(&context, frame_message, &ignored);

  if (!status.ok()) {
    LOG(INFO) << "gRPC error(SendFrame): " << status.error_message();
  }
}
