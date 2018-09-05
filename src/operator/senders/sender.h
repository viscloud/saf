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

// Send metadata to remote endpoint

#ifndef SAF_OPERATOR_SENDERS_SENDER_H_
#define SAF_OPERATOR_SENDERS_SENDER_H_

#include "common/context.h"
#include "operator/operator.h"

class BaseSender {
 public:
  BaseSender() {}
  virtual ~BaseSender() {}
  virtual bool Init() = 0;
  virtual void Send(const std::string& str, const std::string& aux = "") = 0;
};

class Sender : public Operator {
 public:
  Sender(const std::string& endpoint, const std::string& package_type,
         size_t batch_size);
  static std::shared_ptr<Sender> Create(const FactoryParamsType& params);
  static std::string GetSourceName(int index) {
    return "input" + std::to_string(index);
  }
  static std::string GetSinkName(int index) {
    return "output" + std::to_string(index);
  }

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  void Send(std::unique_ptr<Frame> frame);

 private:
  std::string endpoint_;
  std::string package_type_;
  size_t batch_size_;
  std::unique_ptr<BaseSender> sender_;
};

#endif  // SAF_OPERATOR_SENDERS_SENDER_H_
