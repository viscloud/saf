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

#include "operator/image_classifier.h"

#include <stdexcept>

#include "model/model_manager.h"
#include "utils/math_utils.h"
#include "utils/string_utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";
ImageClassifier::ImageClassifier(const ModelDesc& model_desc,
                                 const Shape& input_shape, size_t num_labels,
                                 size_t batch_size)
    : NeuralNetConsumer(OPERATOR_TYPE_IMAGE_CLASSIFIER, model_desc, input_shape,
                        batch_size, {}, {SOURCE_NAME}, {SINK_NAME}),
      layer_(model_desc.GetDefaultOutputLayer()),
      num_labels_(num_labels),
      labels_(LoadLabels(model_desc)) {
  StreamPtr stream = nne_->GetSink("output");
  // Call Operator::SetSource() because NeuralNetConsumer::SetSource() would
  // set the NeuralNetEvaluator's source (because the NeuralNetEvaluator is
  // private).
  Operator::SetSource(SOURCE_NAME, stream);
}

ImageClassifier::ImageClassifier(const ModelDesc& model_desc, size_t num_labels)
    : NeuralNetConsumer(OPERATOR_TYPE_IMAGE_CLASSIFIER, {SOURCE_NAME},
                        {SINK_NAME}),
      layer_(model_desc.GetDefaultOutputLayer()),
      num_labels_(num_labels),
      labels_(LoadLabels(model_desc)) {}

std::shared_ptr<ImageClassifier> ImageClassifier::Create(
    const FactoryParamsType& params) {
  ModelManager& model_manager = ModelManager::GetInstance();
  std::string model_name = params.at("model");
  CHECK(model_manager.HasModel(model_name));
  ModelDesc model_desc = model_manager.GetModelDesc(model_name);
  size_t num_labels = StringToSizet(params.at("num_labels"));

  auto num_channels_pair = params.find("num_channels");
  if (num_channels_pair == params.end()) {
    return std::make_shared<ImageClassifier>(model_desc, num_labels);
  } else {
    // If num_channels is specified, then we need to use the constructor that
    // creates a hidden NeuralNetEvaluator.
    size_t num_channels = StringToSizet(num_channels_pair->second);
    Shape input_shape = Shape(num_channels, model_desc.GetInputWidth(),
                              model_desc.GetInputHeight());
    return std::make_shared<ImageClassifier>(model_desc, input_shape,
                                             num_labels);
  }
}

bool ImageClassifier::Init() { return NeuralNetConsumer::Init(); }

void ImageClassifier::Process() {
  auto frame = GetFrame(SOURCE_NAME);

  if (!frame->Count(layer_)) {
    throw std::runtime_error(
        "ImageClassifiers only operate on a model's default output layer!");
  }
  const cv::Mat& output = frame->GetValue<cv::Mat>(layer_);

  // Assign labels.
  std::vector<Prediction> predictions;
  float* scores;
  // Currently we only support contiguously allocated cv::Mat. Considering this
  // cv::Mat should be small (e.g. 1x1000), it is most likely contiguous.
  if (output.isContinuous()) {
    scores = (float*)(output.data);
  } else {
    LOG(FATAL)
        << "Non-contiguous allocation of cv::Mat is currently not supported";
  }
  // using labels_.size() completely defeats the purpose and also causes issues
  // elsewhere
  // TODO: output.channels() is the correct thing to use, but it may not work
  // correctly for caffe and will also fail in most cases because of the channel
  // limit in opencv
  std::vector<int> top_label_idxs =
      Argmax(scores, output.channels(), num_labels_);
  for (decltype(num_labels_) i = 0; i < num_labels_; ++i) {
    int label_idx = top_label_idxs.at(i);
    predictions.push_back(
        std::make_pair(labels_.at(label_idx), scores[label_idx]));
  }

  // Add the metadata to the frame.
  std::vector<std::string> tags;
  std::vector<double> probabilities;
  for (const auto& pred : predictions) {
    tags.push_back(pred.first);
    probabilities.push_back(pred.second);
  }

  frame->SetValue("tags", tags);
  frame->SetValue("probabilities", probabilities);

  PushFrame(SINK_NAME, std::move(frame));
}

std::vector<std::string> ImageClassifier::LoadLabels(
    const ModelDesc& model_desc) {
  // Load labels
  std::string labels_filepath = model_desc.GetLabelFilePath();
  CHECK(labels_filepath != "") << "Empty label file: " << labels_filepath;
  std::ifstream labels_stream(labels_filepath);
  CHECK(labels_stream) << "Unable to open labels file: " << labels_filepath;

  std::string line;
  std::vector<std::string> labels;
  while (std::getline(labels_stream, line)) {
    labels.push_back(std::string(line));
  }
  return labels;
}
