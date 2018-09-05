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

// Send metadata over Kafka

#ifndef SAF_OPERATOR_SENDERS_MQTT_SENDER_H_
#define SAF_OPERATOR_SENDERS_MQTT_SENDER_H_

#include <iostream>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <mqtt_client_cpp.hpp>

#include "operator/senders/sender.h"

class MqttSender : public BaseSender {
  using Client = std::shared_ptr<mqtt::client<mqtt::tcp_endpoint<
      boost::asio::basic_stream_socket<
          boost::asio::ip::tcp,
          boost::asio::stream_socket_service<boost::asio::ip::tcp>>,
      boost::asio::io_service::strand>>>;

 public:
  MqttSender(const std::string& endpoint) : connected_(false) {
    std::size_t found = endpoint.substr(7).find(":");
    broker_ = endpoint.substr(7).substr(0, found);
    port_ = boost::lexical_cast<std::uint16_t>(
        endpoint.substr(7).substr(found + 1));
  }
  virtual ~MqttSender() {
    if (sender_thread_.joinable()) {
      client_->disconnect();
      sender_thread_.join();
    }
  }
  virtual bool Init() {
    id_ = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
    client_ = mqtt::make_client(ios_, broker_, port_);
    client_->set_client_id(id_);
    client_->set_clean_session(true);
    // Setup handlers
    client_->set_connack_handler(
        [this](bool sp, std::uint8_t connack_return_code) {
          (void)sp;
          if (connack_return_code == mqtt::connect_return_code::accepted) {
            connected_ = true;
          } else {
            LOG(ERROR) << "Connack Return Code: "
                       << mqtt::connect_return_code_to_str(connack_return_code);
          }
          return true;
        });

    client_->connect();

    sender_thread_ = std::thread([this]() { ios_.run(); });
    return true;
  }
  virtual void Send(const std::string& str, const std::string& aux) {
    if (connected_) {
      client_->async_publish_at_least_once(
          "saf" + (aux.empty() ? "" : "/" + aux), str);
      LOG(INFO) << "MQTT client sent " << str.length() << " bytes";
    }
  }

 private:
  std::string id_;
  std::string broker_;
  int port_;
  Client client_;
  boost::asio::io_service ios_;
  std::thread sender_thread_;
  bool connected_;
};

#endif  // SAF_OPERATOR_SENDERS_MQTT_SENDER_H_
