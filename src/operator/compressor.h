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

#ifndef SAF_OPERATOR_COMPRESSOR_H_
#define SAF_OPERATOR_COMPRESSOR_H_

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>

#include "common/types.h"
#include "operator/operator.h"

class Compressor : public Operator {
 public:
  enum CompressionType { BZIP2, GZIP, NONE };

  Compressor(CompressionType t);
  ~Compressor();
  static std::shared_ptr<Compressor> Create(const FactoryParamsType& params);
  static std::string CompressionTypeToString(CompressionType type);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

  static const char* kDataKey;
  static const char* kTypeKey;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  void OutputFrames();
  std::unique_ptr<Frame> CompressFrame(std::unique_ptr<Frame>);

  CompressionType compression_type_;
  std::queue<std::future<std::unique_ptr<Frame>>> queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cond_;
  std::thread output_thread_;
  std::atomic<bool> stop_;
};

#endif  // SAF_OPERATOR_COMPRESSOR_H_
