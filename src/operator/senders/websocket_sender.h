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

// Send metadata to websocket server

#ifndef SAF_OPERATOR_SENDERS_WEBSOCKET_SENDER_H_
#define SAF_OPERATOR_SENDERS_WEBSOCKET_SENDER_H_

#include <iostream>

#include <cpprest/interopstream.h>
#include <cpprest/ws_client.h>

#include "operator/senders/sender.h"

using web::websockets::client::websocket_client;
using web::websockets::client::websocket_incoming_message;
using web::websockets::client::websocket_outgoing_message;

class WebsocketSender : public BaseSender {
 public:
  WebsocketSender(const std::string& endpoint) : endpoint_(endpoint) {}
  virtual ~WebsocketSender() {
    if (!endpoint_.empty() && wclient_) {
      wclient_->close().wait();
    }
  }
  virtual bool Init() {
    wclient_ = std::make_unique<websocket_client>();
    connect_task_ = wclient_->connect(endpoint_);
    return true;
  }
  virtual void Send(const std::string& str, const std::string&) {
    if (connect_task_.is_done()) {
      std::stringstream ss(str);
      Concurrency::streams::stdio_istream<uint8_t> out_buf = ss;
      websocket_outgoing_message out_msg;
      out_msg.set_binary_message(out_buf, str.length());
      LOG(INFO) << "Websocket client send " << str.length() << " bytes";
      wclient_->send(out_msg).wait();
    } else {
      LOG(INFO) << "Websocket client is connecting server ...";
    }
  }

 private:
  std::string endpoint_;
  std::unique_ptr<websocket_client> wclient_;
  pplx::task<void> connect_task_;
};

#endif  // SAF_OPERATOR_SENDERS_WEBSOCKET_SENDER_H_
