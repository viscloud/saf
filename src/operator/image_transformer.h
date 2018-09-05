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

#ifndef SAF_OPERATOR_IMAGE_TRANSFORMER_H_
#define SAF_OPERATOR_IMAGE_TRANSFORMER_H_

#include "common/types.h"
#include "operator/operator.h"
#include "stream/stream.h"

class ImageTransformer : public Operator {
 public:
  ImageTransformer(const Shape& target_shape, bool crop,
                   unsigned int angle = 0);

  static std::shared_ptr<ImageTransformer> Create(
      const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

  static const char* kOutputKey;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  Shape target_shape_;
  bool crop_;
  unsigned int angle_;
  std::string field_;
};

#endif  // SAF_OPERATOR_IMAGE_TRANSFORMER_H_
