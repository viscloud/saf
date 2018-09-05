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

#ifndef SAF_OPERATOR_NEURAL_NET_CONSUMER_H_
#define SAF_OPERATOR_NEURAL_NET_CONSUMER_H_

#include "common/types.h"
#include "model/model.h"
#include "operator/neural_net_evaluator.h"
#include "operator/operator.h"

// This virtual class serves as an interface for operators that wish to use a
// the output of a NeuralNetEvaluator. A NeuralNetConsumer can either use an
// existing NeuralNetEvaluator or automatically initialize a new
// NeuralNetEvaluator. Any number of sources and sinks are supported. For an
// example of how to implement a NeuralNetConsumer, look at the ImageClassifier
// class.
class NeuralNetConsumer : public Operator {
 public:
  // Automatically constructs a NeuralNetEvaluator, which will be hidden and
  // managed automatically.
  NeuralNetConsumer(OperatorType type, const ModelDesc& model_desc,
                    const Shape& input_shape, size_t batch_size = 1,
                    const std::vector<std::string>& output_layer_names = {},
                    const std::vector<std::string>& source_names = {},
                    const std::vector<std::string>& sink_names = {});
  // Assumes that the calling code will construct and connect a
  // NeuralNetEvaluator, which will not be managed automatically.
  NeuralNetConsumer(OperatorType type,
                    const std::vector<std::string>& source_names = {},
                    const std::vector<std::string>& sink_names = {});

  virtual void SetSource(const std::string& name, StreamPtr stream) override;
  virtual void SetBlockOnPush(bool block) override;
  virtual double GetTrailingAvgProcessingLatencyMs() const override;
  virtual double GetAvgProcessingLatencyMs() const override;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  // Returns true if this NeuralNetConsumer created the NeuralNetEvaluator
  // that precedes it (meaning that the NeuralNetEvaluator is private and must
  // be managed by the NeuralNetConsumer), or false otherwise.
  bool NneIsPrivate() const;

  std::vector<std::string> output_layer_names_;
  NeuralNetEvaluator* nne_;
};

#endif  // SAF_OPERATOR_NEURAL_NET_CONSUMER_H_
