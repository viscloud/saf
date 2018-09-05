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

// Face feature extractor using facenet

#include "operator/caffe_facenet.h"

#include <cv.h>

#include "common/context.h"
#include "model/model_manager.h"
#include "utils/utils.h"

Facenet::Facenet(const ModelDesc& model_desc, Shape input_shape,
                 size_t batch_size)
    : Operator(OPERATOR_TYPE_FACENET, {}, {}),
      model_desc_(model_desc),
      input_shape_(input_shape),
      batch_size_(batch_size),
      face_batch_size_(1) {
  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({"input" + std::to_string(i), nullptr});
    sinks_.insert(
        {"output" + std::to_string(i), std::shared_ptr<Stream>(new Stream)});
  }

  mean_image_ = cv::Mat(cv::Size(input_shape_.width, input_shape_.height),
                        input_shape_.channel == 3 ? CV_32FC3 : CV_32FC1,
                        cv::Scalar(127.5, 127.5, 127.5));
  LOG(INFO) << "batch size of " << batch_size_;
}

std::shared_ptr<Facenet> Facenet::Create(const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

bool Facenet::Init() {
  // Set Caffe backend
  int desired_device_number = Context::GetContext().GetInt(DEVICE_NUMBER);

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
  net_ = std::make_unique<caffe::Net<float>>(model_desc_.GetModelDescPath(),
                                             caffe::TEST,
                                             caffe::Caffe::GetDefaultDevice());
#else
  net_ = std::make_unique<caffe::Net<float>>(model_desc_.GetModelDescPath(),
                                             caffe::TEST);
#endif  // USE_OPENCL
  net_->CopyTrainedLayersFrom(model_desc_.GetModelParamsPath());

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  CHECK(input_shape_.channel == 3 || input_shape_.channel == 1)
      << "Input layer should have 1 or 3 channels.";

  caffe::Blob<float>* input_layer = net_->input_blobs()[0];
  // Adjust input dimensions
  input_layer->Reshape(face_batch_size_ /*batch_size_*/, input_shape_.channel,
                       input_shape_.height, input_shape_.width);
  // Forward dimension change to all layers.
  net_->Reshape();
  // Prepare input buffer
  input_layer = net_->input_blobs()[0];
  float* input_data = input_layer->mutable_cpu_data();

  input_buffer_ = input_data;

  LOG(INFO) << "Facenet initialized";
  return true;
}

bool Facenet::OnStop() {
  net_ = nullptr;
  return true;
}

#define GET_SOURCE_NAME(i) ("input" + std::to_string(i))
#define GET_SINK_NAME(i) ("output" + std::to_string(i))

void Facenet::Process() {
  Timer timer;
  timer.Start();

  cv::Size input_geometry(input_shape_.width, input_shape_.height);
  std::vector<std::unique_ptr<Frame>> frames;
  size_t face_total_num = 0;
  for (size_t i = 0; i < batch_size_; i++) {
    auto frame = GetFrame(GET_SOURCE_NAME(i));
    auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    face_total_num += bboxes.size();
    frames.push_back(std::move(frame));
  }

  std::vector<std::vector<float>> face_features;
  if (face_total_num > 0) {
    // reshape
    if (face_batch_size_ != face_total_num) {
      face_batch_size_ = face_total_num;
      caffe::Blob<float>* input_layer = net_->input_blobs()[0];
      // Adjust input dimensions
      input_layer->Reshape(face_batch_size_ /*batch_size_*/,
                           input_shape_.channel, input_shape_.height,
                           input_shape_.width);
      // Forward dimension change to all layers.
      net_->Reshape();
      // Prepare input buffer
      input_layer = net_->input_blobs()[0];
      float* input_data = input_layer->mutable_cpu_data();

      input_buffer_ = input_data;
    }
    float* data = (float*)input_buffer_;

    // load data
    for (size_t i = 0; i < batch_size_; i++) {
      cv::Mat img = frames[i]->GetValue<cv::Mat>("original_image");
      auto bboxes = frames[i]->GetValue<std::vector<Rect>>("bounding_boxes");
      for (const auto& m : bboxes) {
        int x = m.px;
        int y = m.py;
        int w = m.width;
        int h = m.height;
        CHECK((x >= 0) && (y >= 0) && (x + w <= img.cols) &&
              (y + h <= img.rows));
        cv::Rect roi(x, y, w, h);
        face_image_ = img(roi);

        // Resize
        if (face_image_.size() != input_geometry)
          cv::resize(face_image_, face_image_resized_, input_geometry);
        else
          face_image_resized_ = face_image_;

        if (input_shape_.channel == 3)
          face_image_resized_.convertTo(face_image_float_, CV_32FC3);
        else
          face_image_resized_.convertTo(face_image_float_, CV_32FC1);

        cv::subtract(face_image_float_, mean_image_, face_image_subtract_);

        if (input_shape_.channel == 3)
          face_image_subtract_.convertTo(face_image_normalized_, CV_32FC3,
                                         1.0 / 128);
        else
          face_image_subtract_.convertTo(face_image_normalized_, CV_32FC1,
                                         1.0 / 128);

        cv::cvtColor(face_image_normalized_, face_image_bgr_,
                     cv::COLOR_RGB2BGR);

        std::vector<cv::Mat> output_channels;
        for (int j = 0; j < input_shape_.channel; j++) {
          cv::Mat channel(input_shape_.height, input_shape_.width, CV_32FC1,
                          data);
          output_channels.push_back(channel);
          data += input_shape_.width * input_shape_.height;
        }
        cv::split(face_image_bgr_, output_channels);
      }
    }

    CHECK(net_->input_blobs()[0]->mutable_cpu_data() == input_buffer_);

    net_->Forward();
    auto output_blob = net_->output_blobs()[0];
    float* output_data = output_blob->mutable_cpu_data();
    int shape0 = output_blob->shape(0);
    int shape1 = output_blob->shape(1);
    for (int i = 0; i < shape0; ++i) {
      std::vector<float> face_feature(&output_data[i * shape1],
                                      &output_data[i * shape1] + shape1);
      face_features.push_back(face_feature);
    }
  }

  for (size_t i = 0; i < batch_size_; i++) {
    frames[i]->SetValue("face_features", face_features);
    PushFrame(GET_SINK_NAME(i), std::move(frames[i]));
  }

  LOG(INFO) << "Facenet took " << timer.ElapsedMSec() << " ms";
}

void Facenet::SetInputStream(int src_id, StreamPtr stream) {
  SetSource(GET_SOURCE_NAME(src_id), stream);
}
