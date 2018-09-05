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

// Receive metadata over MQTT

#ifndef SAF_OPERATOR_RECEIVERS_MQTT_RECEIVER_H_
#define SAF_OPERATOR_RECEIVERS_MQTT_RECEIVER_H_

#include <iostream>
#include <thread>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mqtt_client_cpp.hpp>

#include "operator/receivers/receiver.h"

class MqttReceiver : public BaseReceiver {
  using Client = std::shared_ptr<mqtt::client<mqtt::tcp_endpoint<
      boost::asio::basic_stream_socket<
          boost::asio::ip::tcp,
          boost::asio::stream_socket_service<boost::asio::ip::tcp>>,
      boost::asio::io_service::strand>>>;

 public:
  MqttReceiver(const std::string& endpoint, const std::string& aux) {
    aux_ = aux;
    std::size_t found = endpoint.substr(7).find(":");
    broker_ = endpoint.substr(7).substr(0, found);
    port_ = boost::lexical_cast<std::uint16_t>(
        endpoint.substr(7).substr(found + 1));
  }
  virtual ~MqttReceiver() {
    if (receiver_thread_.joinable()) {
      client_->disconnect();
      receiver_thread_.join();
    }
  }
  virtual bool Init() {
    id_ = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
    client_ = mqtt::make_client(ios_, broker_, port_);
    // Setup client
    client_->set_client_id(id_);
    client_->set_clean_session(true);
    // Setup handlers
    client_->set_connack_handler(
        [this](bool sp, std::uint8_t connack_return_code) {
          (void)sp;
          if (connack_return_code == mqtt::connect_return_code::accepted) {
            pid_sub1_ = client_->subscribe("saf/#", mqtt::qos::at_least_once);
          }
          return true;
        });
    client_->set_close_handler([]() { LOG(INFO) << "closed."; });
    client_->set_error_handler([](boost::system::error_code const& ec) {
      LOG(INFO) << "error: " << ec.message();
    });
    client_->set_publish_handler(
        [&](std::uint8_t header, boost::optional<std::uint16_t> packet_id,
            std::string topic_name, std::string contents) {
          (void)header;
          (void)packet_id;
          (void)topic_name;
          // Push the content
          {
            // exclude receiving own data stream
            std::string cont_name = "saf/" + aux_;

            if (topic_name != cont_name) {
              std::lock_guard<std::mutex> guard(receiver_lock_);

              auto ss = std::make_unique<std::stringstream>(contents);
              stringstreams_.push(std::move(ss));
            }
          }
          receiver_cv_.notify_all();
          return true;
        });

    client_->connect();

    receiver_thread_ = std::thread([this]() { ios_.run(); });
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
  std::string aux_;
  std::string id_;
  std::string broker_;
  int port_;
  Client client_;
  boost::asio::io_service ios_;

  std::mutex receiver_lock_;
  std::condition_variable receiver_cv_;
  std::thread receiver_thread_;
  std::uint16_t pid_sub1_;
  std::queue<std::unique_ptr<std::stringstream>> stringstreams_;
};

#endif  // SAF_OPERATOR_RECEIVERS_MQTT_RECEIVER_H_
