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

// Receive metadata from Kafka

#ifndef SAF_OPERATOR_RECEIVERS_KAFKA_RECEIVER_H_
#define SAF_OPERATOR_RECEIVERS_KAFKA_RECEIVER_H_

#include <iostream>
#include <map>

#include <librdkafka/src-cpp/rdkafkacpp.h>

#include "operator/receivers/receiver.h"
#include "utils/hash_utils.h"

class KafkaReceiver : public BaseReceiver {
 public:
  KafkaReceiver(const std::string& endpoint)
      : conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
        tconf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)) {
    std::string errstr;
    conf_->set("metadata.broker.list", endpoint.substr(8), errstr);
    conf_->set("message.max.bytes", "1000000000", errstr);
    conf_->set("socket.send.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.receive.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.request.max.bytes", "1000000000", errstr);
  }
  virtual ~KafkaReceiver() {}
  virtual bool Init() {
    std::string errstr;
    consumer_.reset(RdKafka::Consumer::create(conf_.get(), errstr));
    return consumer_ != nullptr;
  }
  virtual std::unique_ptr<std::stringstream> Receive(const std::string& aux) {
    if (consumer_.get() == nullptr) {
      LOG(FATAL) << "Kafka consumer was not initialized.";
    }
    std::string errstr;
    std::string topic_str = "saf" + (aux.empty() ? "" : "-" + aux);
    if (topics_.find(topic_str) == topics_.end()) {
      topics_[topic_str] =
          std::unique_ptr<RdKafka::Topic>(RdKafka::Topic::create(
              consumer_.get(), topic_str, tconf_.get(), errstr));
      if (topics_[topic_str].get() == nullptr) {
        LOG(FATAL) << "Failed to create topic: " << errstr;
      }
      RdKafka::ErrorCode resp = consumer_->start(topics_[topic_str].get(), 0,
                                                 RdKafka::Topic::OFFSET_END);
      if (resp != RdKafka::ERR_NO_ERROR) {
        LOG(INFO) << "Kafka consume failed: " << RdKafka::err2str(resp);
      }
    }
    std::unique_ptr<std::stringstream> ret;
    while (true) {
      RdKafka::Message* msg =
          consumer_->consume(topics_[topic_str].get(), 0, 1000);
      if (msg->err() == RdKafka::ERR_NO_ERROR) {
        LOG(INFO) << "Kafka polls from topic " << topic_str;
        LOG(INFO) << "Kafka reads message at offset " << msg->offset();
        LOG(INFO) << "Kafka received " << static_cast<int>(msg->len())
                  << " bytes";
        std::string str((char*)msg->payload(), msg->len());
        ret.reset(new std::stringstream(str));
        delete msg;
        break;
      }
      delete msg;
    }
    consumer_->poll(0);
    return ret;
  }

 private:
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> tconf_;
  std::unique_ptr<RdKafka::Consumer> consumer_;
  std::map<std::string, std::unique_ptr<RdKafka::Topic>> topics_;
};

#endif  // SAF_OPERATOR_RECEIVERS_KAFKA_RECEIVER_H_
