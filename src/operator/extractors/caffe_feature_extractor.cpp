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

// Feature extractor using caffe

#include "operator/extractors/caffe_feature_extractor.h"

#include <cv.h>

#include "common/context.h"
#include "model/model_manager.h"
#include "utils/cv_utils.h"

bool CaffeCNNFeatureExtractor::Init() {
  std::string model_file = model_desc_.GetModelDescPath();
  std::string weights_file = model_desc_.GetModelParamsPath();
  LOG(INFO) << "model_file: " << model_file;
  LOG(INFO) << "weights_file: " << weights_file;

  // Set Caffe backend
  int desired_device_number = model_desc_.GetDevice()
                                  ? *(model_desc_.GetDevice())
                                  : Context::GetContext().GetInt(DEVICE_NUMBER);
  LOG(INFO) << "desired_device_number: " << desired_device_number;

  if (desired_device_number == DEVICE_NUMBER_CPU_ONLY) {
    LOG(INFO) << "Use device: " << desired_device_number << "(CPU)";
    caffe::Caffe::set_mode(caffe::Caffe::CPU);
  } else {
#ifdef USE_CUDA
    std::vector<int> gpus;
    GetCUDAGpus(gpus);

    if (desired_device_number < (int)gpus.size()) {
      // Device exists
      LOG(INFO) << "Use GPU with device ID " << desired_device_number;
      caffe::Caffe::SetDevice(desired_device_number);
      caffe::Caffe::set_mode(caffe::Caffe::GPU);
    } else {
      LOG(FATAL) << "No GPU device: " << desired_device_number;
    }
#elif USE_OPENCL
    std::vector<int> gpus;
    int count = caffe::Caffe::EnumerateDevices();

    if (desired_device_number < count) {
      // Device exists
      LOG(INFO) << "Use GPU with device ID " << desired_device_number;
      caffe::Caffe::SetDevice(desired_device_number);
      caffe::Caffe::set_mode(caffe::Caffe::GPU);
    } else {
      LOG(FATAL) << "No GPU device: " << desired_device_number;
    }
#else
    LOG(FATAL) << "Compiled in CPU_ONLY mode but have a device number "
                  "configured rather than -1";
#endif  // USE_CUDA
  }

// Load the network.
#ifdef USE_OPENCL
  net_ = std::make_unique<caffe::Net<float>>(model_file, caffe::TEST,
                                             caffe::Caffe::GetDefaultDevice());
#else
  net_ = std::make_unique<caffe::Net<float>>(model_file, caffe::TEST);
#endif  // USE_OPENCL
  net_->CopyTrainedLayersFrom(model_desc_.GetModelParamsPath());

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  caffe::Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
      << "Input layer should have 1 or 3 channels.";
  input_blob_size_ = cv::Size(input_layer->width(), input_layer->height());

  caffe::TransformationParameter transform_param;
  caffe::ResizeParameter* resize_param = transform_param.mutable_resize_param();
  resize_param->set_resize_mode(caffe::ResizeParameter_Resize_mode_WARP);

  // TODO: To be removed. Incorporating the following into configs.
  transform_param.add_mean_value(104);
  transform_param.add_mean_value(117);
  transform_param.add_mean_value(124);

  resize_param->set_width(input_blob_size_.width);
  resize_param->set_height(input_blob_size_.height);
  resize_param->set_prob(1.0);
  resize_param->add_interp_mode(caffe::ResizeParameter_Interp_mode_LINEAR);
#ifdef USE_OPENCL
  data_transformer_ = std::make_unique<caffe::DataTransformer<float>>(
      transform_param, caffe::TEST, caffe::Caffe::GetDefaultDevice());
#else
  data_transformer_ = std::make_unique<caffe::DataTransformer<float>>(
      transform_param, caffe::TEST);
#endif  // USE_OPENCL

  LOG(INFO) << "FeatureExtractor initialized";
  return true;
}

void CaffeCNNFeatureExtractor::Extract(
    const cv::Mat& image, const std::vector<Rect>& bboxes,
    std::vector<std::vector<double>>& features) {
  size_t bboxes_count = bboxes.size();
  if (bboxes_count > 0) {
    caffe::Blob<float>* input_layer = net_->input_blobs()[0];
    input_layer->Reshape(bboxes_count, num_channels_, input_blob_size_.height,
                         input_blob_size_.width);
    // Forward dimension change to all layers
    net_->Reshape();

    std::vector<cv::Mat> bbox_images;
    for (const auto& m : bboxes) {
      int x = m.px;
      int y = m.py;
      int w = m.width;
      int h = m.height;
      CHECK((x >= 0) && (y >= 0) && (x + w <= image.cols) &&
            (y + h <= image.rows));
      cv::Rect roi(x, y, w, h);
      auto bbox_image = image(roi);
      auto bbox_image_f = FixupChannels(bbox_image, num_channels_);
      bbox_images.push_back(bbox_image_f);
    }
    data_transformer_->Transform(bbox_images, input_layer);
    net_->Forward();

    auto output_blob = net_->output_blobs()[0];
    float* output_data = output_blob->mutable_cpu_data();
    int shape0 = output_blob->shape(0);
    int shape1 = output_blob->shape(1);
    for (int j = 0; j < shape0; ++j) {
      std::vector<double> feature(&output_data[j * shape1],
                                  &output_data[j * shape1] + shape1);
      features.push_back(feature);
    }
  }
}
