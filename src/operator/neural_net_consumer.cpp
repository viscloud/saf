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

#include "operator/neural_net_consumer.h"
#include "utils/utils.h"

NeuralNetConsumer::NeuralNetConsumer(
    OperatorType type, const ModelDesc& model_desc, const Shape& input_shape,
    size_t batch_size, const std::vector<std::string>& output_layer_names,
    const std::vector<std::string>& source_names,
    const std::vector<std::string>& sink_names)
    : Operator(type, source_names, sink_names),
      output_layer_names_(output_layer_names),
      nne_(new NeuralNetEvaluator(model_desc, input_shape, batch_size,
                                  output_layer_names)) {}

NeuralNetConsumer::NeuralNetConsumer(
    OperatorType type, const std::vector<std::string>& source_names,
    const std::vector<std::string>& sink_names)
    : Operator(type, source_names, sink_names), nne_(NULL) {}

void NeuralNetConsumer::SetSource(const std::string& name, StreamPtr stream) {
  if (NneIsPrivate()) {
    // If we are managing the NeuralNetEvaluator, then set its source instead.
    nne_->SetSource(name, stream, "");
  } else {
    Operator::SetSource(name, stream);
  }
}

void NeuralNetConsumer::SetBlockOnPush(bool block) {
  if (NneIsPrivate()) {
    // If we are managing the NeuralNetEvaluator, then call SetBlockOnPush() for
    // it as well.
    nne_->SetBlockOnPush(block);
  }
  Operator::SetBlockOnPush(block);
}

double NeuralNetConsumer::GetTrailingAvgProcessingLatencyMs() const {
  double our_latency = Operator::GetTrailingAvgProcessingLatencyMs();
  if (NneIsPrivate()) {
    // We add our latency to the latency of our hidden NeuralNetEvaluator.
    return nne_->GetTrailingAvgProcessingLatencyMs() + our_latency;
  } else {
    return our_latency;
  }
}

double NeuralNetConsumer::GetAvgProcessingLatencyMs() const {
  double our_latency = Operator::GetAvgProcessingLatencyMs();
  if (NneIsPrivate()) {
    // We add our latency to the latency of our hidden NeuralNetEvaluator.
    return nne_->GetAvgProcessingLatencyMs() + our_latency;
  } else {
    return our_latency;
  }
}

bool NeuralNetConsumer::Init() {
  if (NneIsPrivate()) {
    return nne_->Start();
  } else {
    return true;
  }
}

bool NeuralNetConsumer::OnStop() {
  if (NneIsPrivate()) {
    bool result = nne_->Stop();
    delete nne_;
    return result;
  } else {
    return true;
  }
}

bool NeuralNetConsumer::NneIsPrivate() const { return nne_ != NULL; }
