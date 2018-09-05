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

// Receive metadata over Websocket

#ifndef SAF_OPERATOR_RECEIVERS_WEBSOCKET_RECEIVER_H_
#define SAF_OPERATOR_RECEIVERS_WEBSOCKET_RECEIVER_H_

#include <iostream>
#include <thread>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "operator/receivers/receiver.h"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

class WebsocketReceiver : public BaseReceiver {
  typedef websocketpp::server<websocketpp::config::asio> server;
  typedef server::message_ptr message_ptr;

 public:
  WebsocketReceiver(const std::string& endpoint) {
    std::size_t found = endpoint.substr(5).find(":");
    server_addr_ = endpoint.substr(5).substr(0, found);
    port_ = boost::lexical_cast<std::uint16_t>(
        endpoint.substr(5).substr(found + 1));
  }
  virtual ~WebsocketReceiver() {
    if (receiver_thread_.joinable()) {
      server_.stop_listening();
      server_.stop();
      receiver_thread_.join();
    }
  }
  virtual bool Init() {
    // Set logging settings
    server_.set_access_channels(websocketpp::log::alevel::all);
    server_.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize Asio
    server_.init_asio();

    // Register our message handler
    // server_.set_message_handler(bind(&on_message,&server_,::_1,::_2));
    server_.set_message_handler(
        [this](websocketpp::connection_hdl hdl, message_ptr msg) {
          hdl.lock().get();
          // Push the content
          {
            std::lock_guard<std::mutex> guard(receiver_lock_);

            auto ss = std::make_unique<std::stringstream>(msg->get_payload());
            stringstreams_.push(std::move(ss));
          }
          receiver_cv_.notify_all();
          return true;
        });

    // Listen on port 9002
    server_.listen(port_);

    // Start the server accept loop
    server_.start_accept();

    receiver_thread_ = std::thread([this]() {
      // Start the ASIO io_service run loop
      server_.run();
    });
    return true;
  }
  virtual std::unique_ptr<std::stringstream> Receive(const std::string&) {
    std::unique_lock<std::mutex> lk(receiver_lock_);
    bool pred =
        receiver_cv_.wait_for(lk, std::chrono::milliseconds(100), [this] {
          // Stop waiting when frame available
          return stringstreams_.size() != 0;
        });

    if (!pred) {
      // The wait stopped because of a timeout.
      return nullptr;
    }

    auto ss = std::move(stringstreams_.front());
    stringstreams_.pop();
    return ss;
  }

 private:
  std::string server_addr_;
  server server_;
  int port_;

  std::mutex receiver_lock_;
  std::condition_variable receiver_cv_;
  std::thread receiver_thread_;
  std::queue<std::unique_ptr<std::stringstream>> stringstreams_;
};

#endif  // SAF_OPERATOR_RECEIVERS_WEBSOCKET_RECEIVER_H_
