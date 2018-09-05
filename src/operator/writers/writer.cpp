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

// Write metadata to storage

#include "operator/writers/writer.h"

#include <chrono>
#include <iostream>

#include "operator/writers/file_writer.h"
#include "utils/string_utils.h"

Writer::Writer(const std::string& target, const std::string& uri,
               size_t batch_size)
    : Operator(OPERATOR_TYPE_WRITER, {}, {}),
      target_(target),
      uri_(uri),
      batch_size_(batch_size) {
  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({GetSourceName(i), nullptr});
  }
}

std::shared_ptr<Writer> Writer::Create(const FactoryParamsType& params) {
  auto target = params.at("target");
  auto uri = params.at("uri");
  auto batch_size = StringToSizet(params.at("batch_size"));
  return std::make_shared<Writer>(target, uri, batch_size);
}

bool Writer::Init() {
  bool status = false;
  if (target_.empty()) {
    LOG(FATAL) << "Writer target cannot be empty.";
  } else {
    if (target_ == "file") {
      writer_ = std::make_unique<FileWriter>(uri_);
      status = writer_->Init();
    } else {
      LOG(FATAL) << "Writer type not supported.";
    }
  }
  return status;
}

bool Writer::OnStop() {
  writer_ = nullptr;
  return true;
}

void Writer::Process() {
  for (size_t i = 0; i < batch_size_; i++) {
    writer_->Write(GetFrame(GetSourceName(i)));
  }
}
