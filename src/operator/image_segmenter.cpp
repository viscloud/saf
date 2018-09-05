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

#include "operator/image_segmenter.h"

#include <unordered_map>

#include "model/model_manager.h"

ImageSegmenter::ImageSegmenter(const ModelDesc& model_desc, Shape input_shape)
    : Operator(OPERATOR_TYPE_IMAGE_SEGMENTER, {"input"}, {"output"}),
      model_desc_(model_desc),
      input_shape_(input_shape) {}

std::shared_ptr<ImageSegmenter> ImageSegmenter::Create(
    const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

bool ImageSegmenter::Init() {
  // Load model
  auto& manager = ModelManager::GetInstance();
  model_ = manager.CreateModel(model_desc_, input_shape_);
  model_->Load();

  // Create mean image
  auto mean_colors = manager.GetMeanColors();
  mean_image_ =
      cv::Mat(cv::Size(input_shape_.width, input_shape_.height), CV_32FC3,
              cv::Scalar(mean_colors[0], mean_colors[1], mean_colors[2]));

  LOG(INFO) << "Operator initialized";
  return true;
}

bool ImageSegmenter::OnStop() {
  model_ = nullptr;
  return true;
}

void ImageSegmenter::Process() {
  // Do image segmentation
  Timer timer;
  timer.Start();
  auto frame = GetFrame("input");
  const cv::Mat& image = frame->GetValue<cv::Mat>("image");
  const cv::Mat& original_image = frame->GetValue<cv::Mat>("original_image");

  CHECK(image.channels() == input_shape_.channel &&
        image.size[0] == input_shape_.width &&
        image.size[1] == input_shape_.height);

  // Get bytes to feed into the model
  std::vector<cv::Mat> output_channels;

  std::unordered_map<std::string, std::vector<cv::Mat>> input_map;
  input_map[model_desc_.GetDefaultInputLayer()] = {image};
  auto layer_outputs =
      model_->Evaluate(input_map, {model_desc_.GetDefaultOutputLayer()});

  // Expect only 1 output, being the last layer of a non-batched neural net
  CHECK(layer_outputs.size() == 1);
  cv::Mat output = layer_outputs.begin()->second.at(0);
  CHECK(output.dims == 3);
  std::vector<cv::Mat> output_split;
  // TODO: double check this code, not sure how channels work in this case
  for (decltype(output.channels()) i = 0; i < output.channels(); ++i) {
    // This seems a bit reversed, double check!!!
    cv::Mat channel(output.size[1], output.size[0], CV_32FC1);
    output_split.push_back(channel);
  }
  cv::split(output, output_split);

  // Render the segmentation
  cv::Mat output_img = cv::Mat::zeros(output.size[1], output.size[0], CV_8U);
  cv::Mat output_score = cv::Mat::zeros(output.size[1], output.size[0], CV_32F);
  for (const auto& channel : output_split) {
    CHECK(channel.dims == output_img.dims);
    CHECK(channel.size[0] == output_img.size[0]);
    CHECK(channel.size[1] == output_img.size[1]);
    auto channel_it = channel.begin<float>(),
         channel_end = channel.end<float>();
    cv::MatIterator_<uchar> output_img_it = output_img.begin<uchar>(),
                            output_img_end = output_img.end<uchar>();
    cv::MatIterator_<float> output_score_it = output_score.begin<float>(),
                            output_score_end = output_score.end<float>();
    for (uchar channel_number = 0;
         channel_it != channel_end && output_img_it != output_img_end &&
         output_score_it != output_score_end;
         ++channel_it, ++output_img_it, ++output_score_it, ++channel_number) {
      if (*channel_it > *output_score_it) {
        *output_score_it = *channel_it;
        *output_img_it = channel_number;
      }
    }
  }

  cv::Mat output_frame;
  cv::Mat colored_output;
  output_img.convertTo(output_frame, CV_8U, 255.0 / 21);

  cv::applyColorMap(output_frame, colored_output, 4);
  cv::resize(colored_output, colored_output,
             cv::Size(original_image.cols, original_image.rows));

  frame->SetValue("image", colored_output);
  PushFrame("output", std::move(frame));
  LOG(INFO) << "Segmentation takes " << timer.ElapsedMSec() << " ms";
}
