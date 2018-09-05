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

// Multi-target detection using Caffe Yolo

#include "operator/detectors/caffe_yolo_detector.h"

#include "common/context.h"
#include "model/model_manager.h"
#include "utils/yolo_utils.h"

namespace yolo {
Detector::Detector(const std::string& model_file,
                   const std::string& weights_file) {
  // Set Caffe backend
  int desired_device_number = Context::GetContext().GetInt(DEVICE_NUMBER);

  if (desired_device_number == DEVICE_NUMBER_CPU_ONLY) {
    LOG(INFO) << "Use device: " << desired_device_number << "(CPU)";
    caffe::Caffe::set_mode(caffe::Caffe::CPU);
  } else {
#ifdef USE_CUDA
    std::vector<int> gpus;
    GetCUDAGpus(gpus);

    if (desired_device_number < gpus.size()) {
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

// Load the network
#ifdef USE_OPENCL
  net_ = std::make_unique<caffe::Net<float>>(model_file, caffe::TEST,
                                             caffe::Caffe::GetDefaultDevice());
#else
  net_ = std::make_unique<caffe::Net<float>>(model_file, caffe::TEST);
#endif  // USE_OPENCL
  net_->CopyTrainedLayersFromBinaryProto(weights_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  caffe::Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
      << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());
}

std::vector<float> Detector::Detect(const cv::Mat& img) {
  caffe::Blob<float>* input_layer = net_->input_blobs()[0];
  int width = input_layer->width();
  int height = input_layer->height();
  int size = width * height;
  cv::Mat image_resized;
  cv::resize(img, image_resized, cv::Size(height, width));

  float* input_data = input_layer->mutable_cpu_data();
  int temp, idx;
  for (int i = 0; i < height; ++i) {
    uchar* pdata = image_resized.ptr<uchar>(i);
    for (int j = 0; j < width; ++j) {
      temp = 3 * j;
      idx = i * width + j;
      input_data[idx] = (pdata[temp + 2] / 127.5) - 1;
      input_data[idx + size] = (pdata[temp + 1] / 127.5) - 1;
      input_data[idx + 2 * size] = (pdata[temp + 0] / 127.5) - 1;
    }
  }

  net_->Forward();

  caffe::Blob<float>* output_layer = net_->output_blobs()[0];
  const float* begin = output_layer->cpu_data();
  const float* end = begin + output_layer->channels();
  std::vector<float> DetectionResult(begin, end);
  return DetectionResult;
}
}  // namespace yolo

bool YoloDetector::Init() {
  std::string model_file = model_desc_.GetModelDescPath();
  std::string weights_file = model_desc_.GetModelParamsPath();
  LOG(INFO) << "model_file: " << model_file;
  LOG(INFO) << "weights_file: " << weights_file;
  auto mean_colors = ModelManager::GetInstance().GetMeanColors();
  std::ostringstream mean_colors_stream;
  mean_colors_stream << mean_colors[0] << "," << mean_colors[1] << ","
                     << mean_colors[2];

  detector_ = std::make_unique<yolo::Detector>(model_file, weights_file);
  std::string labelmap_file = model_desc_.GetLabelFilePath();
  voc_names_ = ReadVocNames(labelmap_file);

  LOG(INFO) << "YoloDetector initialized";
  return true;
}

std::vector<ObjectInfo> YoloDetector::Detect(const cv::Mat& image) {
  std::vector<ObjectInfo> result;
  std::vector<float> DetectionOutput = detector_->Detect(image);
  std::vector<std::vector<int>> bboxs;
  float pro_obj[49][2];
  int idx_class[49];
  std::vector<std::vector<int>> bboxes =
      GetBoxes(DetectionOutput, &pro_obj[0][0], idx_class, bboxs, 0.01, image);

  for (const auto& m : bboxes) {
    ObjectInfo object_info;
    object_info.tag = voc_names_.at(m[0]);
    object_info.bbox = cv::Rect(m[1], m[2], (m[3] - m[1]), (m[4] - m[2]));
    object_info.confidence = m[5] * 1.0 / 100;
    result.push_back(object_info);
  }

  return result;
}
