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

// Multi-target detection using FRCNN

#include "operator/detectors/frcnn_detector.h"

#include <caffe/FRCNN/util/frcnn_vis.hpp>

#include "common/context.h"
#include "model/model_manager.h"

bool FrcnnDetector::Init() {
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

  std::string proto_file = model_desc_.GetModelDescPath();
  std::string model_file = model_desc_.GetModelParamsPath();
  std::string voc_config = model_desc_.GetVocConfigPath();
  LOG(INFO) << "proto_file: " << proto_file;
  LOG(INFO) << "model_file: " << model_file;
  LOG(INFO) << "voc_config: " << voc_config;

  API::Set_Config(voc_config);
  detector_ = std::make_unique<API::Detector>(proto_file, model_file);

  LOG(INFO) << "ObjectDetector initialized";
  return true;
}

virtual std::vector<ObjectInfo> FrcnnDetector::Detect(const cv::Mat& image) {
  std::vector<caffe::Frcnn::BBox<float>> results;
  detector_->predict(image, results);

  std::vector<ObjectInfo> result;
  for (const auto& m : results) {
    ObjectInfo object_info;
    object_info.tag =
        caffe::Frcnn::GetClassName(caffe::Frcnn::LoadVocClass(), m.id);
    object_info.bbox = cv::Rect(m[0], m[1], (m[2] - m[0]), (m[3] - m[1]));
    object_info.confidence = m.confidence;
    result.push_back(object_info);
  }

  return result;
}
