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

#ifndef SAF_OPERATOR_BUFFER_H_
#define SAF_OPERATOR_BUFFER_H_

#include <memory>

#include <boost/circular_buffer.hpp>

#include "common/types.h"
#include "operator/operator.h"

// This operator stores a configurable number of recent frames, effectively
// introducing a delay in the pipeline.
class Buffer : public Operator {
 public:
  Buffer(unsigned long num_frames);
  static std::shared_ptr<Buffer> Create(const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  boost::circular_buffer<std::unique_ptr<Frame>> buffer_;
};

#endif  // SAF_OPERATOR_BUFFER_H_
