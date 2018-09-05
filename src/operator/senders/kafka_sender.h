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

#ifndef SAF_OPERATOR_SENDERS_KAFKA_SENDER_H_
#define SAF_OPERATOR_SENDERS_KAFKA_SENDER_H_

#include <iostream>
#include <map>

#include <librdkafka/src-cpp/rdkafkacpp.h>

#include "operator/senders/sender.h"
#include "utils/hash_utils.h"

class KafkaSender : public BaseSender {
 public:
  KafkaSender(const std::string& endpoint)
      : conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
        tconf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)) {
    std::string errstr;
    conf_->set("metadata.broker.list", endpoint.substr(8), errstr);
    conf_->set("batch.size", "1048576", errstr);
    conf_->set("acks", "0", errstr);
    conf_->set("message.max.bytes", "1000000000", errstr);
    conf_->set("socket.send.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.receive.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.request.max.bytes", "1000000000", errstr);
  }
  virtual ~KafkaSender() {
    if (producer_.get() != nullptr) {
      while (producer_->outq_len() > 0) {
        producer_->poll(1000);
      }
    }
  }
  virtual bool Init() {
    std::string errstr;
    producer_.reset(RdKafka::Producer::create(conf_.get(), errstr));
    return producer_.get() != nullptr;
  }
  virtual void Send(const std::string& str, const std::string& aux) {
    if (producer_.get() == nullptr) {
      LOG(FATAL) << "Kafka producer was not initialized.";
    }
    std::string errstr;
    std::string topic_str = "saf" + (aux.empty() ? "" : "-" + aux);
    if (topics_.find(topic_str) == topics_.end()) {
      topics_[topic_str] =
          std::unique_ptr<RdKafka::Topic>(RdKafka::Topic::create(
              producer_.get(), topic_str, tconf_.get(), errstr));
    }
    if (topics_[topic_str].get() == nullptr) {
      LOG(FATAL) << "Failed to create topic: " << errstr;
    }
    RdKafka::ErrorCode resp = producer_->produce(
        topics_[topic_str].get(), RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY, const_cast<char*>(str.c_str()),
        str.size(), NULL, NULL);
    if (resp != RdKafka::ERR_NO_ERROR) {
      LOG(INFO) << "Kafka produce failed: " << RdKafka::err2str(resp);
    } else {
      LOG(INFO) << "Kafka sent " << str.length() << " bytes to " << topic_str;
    }
    producer_->poll(0);
  }

 private:
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> tconf_;
  std::unique_ptr<RdKafka::Producer> producer_;
  std::map<std::string, std::unique_ptr<RdKafka::Topic>> topics_;
};

#endif  // SAF_OPERATOR_SENDERS_KAFKA_SENDER_H_
