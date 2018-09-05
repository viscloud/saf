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

// Receive metadata from remote endpoint

#ifndef SAF_OPERATOR_RECEIVERS_RECEIVER_H_
#define SAF_OPERATOR_RECEIVERS_RECEIVER_H_

#include "common/context.h"
#include "operator/operator.h"

class BaseReceiver {
 public:
  BaseReceiver() {}
  virtual ~BaseReceiver() {}
  virtual bool Init() = 0;
  virtual std::unique_ptr<std::stringstream> Receive(
      const std::string& aux = "") = 0;
};

class Receiver : public Operator {
 public:
  Receiver(const std::string& endpoint, const std::string& package_type,
           const std::string& aux = "");
  static std::shared_ptr<Receiver> Create(const FactoryParamsType& params);
  static std::string GetSinkName() { return "output"; }

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string aux_;
  std::string endpoint_;
  std::string package_type_;
  std::unique_ptr<BaseReceiver> receiver_;
};

#endif  // SAF_OPERATOR_RECEIVERS_RECEIVER_H_
