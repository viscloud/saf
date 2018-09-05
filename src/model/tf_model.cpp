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

#include "model/tf_model.h"

#include <fstream>

#include <tensorflow/core/protobuf/meta_graph.pb.h>

#include "common/context.h"
#include "utils/utils.h"

TFModel::TFModel(const ModelDesc& model_desc, Shape input_shape)
    : Model(model_desc, input_shape),
      input_op_(model_desc.GetDefaultInputLayer()),
      last_op_(model_desc.GetDefaultOutputLayer()) {}

TFModel::~TFModel() {
  tensorflow::Session* raw = session_.release();
  delete raw;
}

void TFModel::Load() {
  int desired_device_number = Context::GetContext().GetInt(DEVICE_NUMBER);
  if (desired_device_number == DEVICE_NUMBER_CPU_ONLY) {
    LOG(INFO) << "Use device: " << DEVICE_NUMBER_CPU_ONLY << " (CPU)";
  } else {
    LOG(FATAL) << "Compiled in CPU-only mode but using a device number "
               << "other than -1.";
  }

  // Load the network.
  tensorflow::GraphDef graph_def;
  tensorflow::Status status = ReadBinaryProto(
      tensorflow::Env::Default(), model_desc_.GetModelDescPath(), &graph_def);
  if (!status.ok()) {
    LOG(FATAL) << "Failed to load TensorFlow graph: " << status.error_message();
  }

  session_.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  status = session_->Create(graph_def);
  if (!status.ok()) {
    LOG(FATAL) << "Failed to create TensorFlow Session: "
               << status.error_message();
  }
}

cv::Mat TFModel::ConvertAndNormalize(cv::Mat img) {
  cv::Mat converted;
  if (input_shape_.channel == 3) {
    img.convertTo(converted, CV_32FC3);
  } else {
    img.convertTo(converted, CV_32FC1);
  }

  cv::Mat normalized;
  cv::normalize(converted, normalized, -0.5, 0.5, cv::NORM_MINMAX);
  return normalized;
}

std::unordered_map<std::string, std::vector<cv::Mat>> TFModel::Evaluate(
    const std::unordered_map<std::string, std::vector<cv::Mat>>& input_map,
    const std::vector<std::string>& output_layer_names) {
  CHECK_EQ(input_map.size(), 1)
      << "Specifying multiple input layers is not supported.";

  std::vector<std::pair<std::string, tensorflow::Tensor>> inputs;
  std::unordered_map<std::string, std::vector<cv::Mat>> ret;

  for (const auto& input_pair : input_map) {
    std::string input_layer_name = input_pair.first;
    std::vector<cv::Mat> input_vec = input_pair.second;

    cv::Mat input = input_vec.at(0);
    int channel = input.channels();
    int height = input.rows;
    int width = input.cols;
    if (input.dims == 4) {
      channel = input.size[3];
      height = input.size[1];
      width = input.size[2];
    }
    // copy data from split (BGR) channels to (RGB) tensor. Datatype must be
    // float. Float16 is not supported yet. Batch size is always 1
    tensorflow::Tensor input_tensor(
        tensorflow::DT_FLOAT,
        tensorflow::TensorShape({static_cast<long long>(input_vec.size()),
                                 height, width, channel}));
    CHECK(input.type() == CV_32FC3) << "Currently, TensorFlow models only "
                                       "support 32-bit floating point data.";
    // TODO: Add handling for non-continuous cv mat
    CHECK(input.isContinuous()) << "cv::Mat must be continuous.";
    // This works because the cv::Mat is stored in HWC format. If we want to
    // support CHW format, then we will need to transpose the tensor. It is not
    // clear whether C++ API exports tf.transpose(). Perhaps this will need to
    // be done using Eigen.
    for (decltype(input_vec.size()) b = 0; b < input_vec.size(); ++b) {
      std::copy_n(
          (float*)input_vec[b].data, channel * height * width,
          input_tensor.flat<float>().data() + b * channel * height * width);
    }
    // If the input layer is not specified, use the default
    if (input_layer_name == "") {
      input_layer_name = model_desc_.GetDefaultInputLayer();
    }
    inputs.push_back(std::pair<std::string, tensorflow::Tensor>(
        input_layer_name, input_tensor));
  }

  // Run inference.
  std::vector<tensorflow::Tensor> outputs;
  tensorflow::Status status =
      session_->Run(inputs, {output_layer_names}, {}, &outputs);

  if (!status.ok()) {
    LOG(FATAL) << "Session::Run() completed with errors: "
               << status.error_message();
  }

  int count = 0;
  std::vector<cv::Mat> return_vector;
  for (const auto& output_tensor : outputs) {
    tensorflow::TensorShape tensor_shape = output_tensor.shape();
    auto dims = tensor_shape.dim_sizes();
    int batch_size = dims[0];
    int channels = 1;
    int height = 1;
    int width = 1;
    auto num_dims = dims.size();
    switch (num_dims) {
      case 2: {
        channels = dims[1];
        break;
      }
      case 4: {
        height = dims[1];
        width = dims[2];
        channels = dims[3];
        break;
      }
      default: {
        LOG(FATAL) << num_dims
                   << " dimensional tensor not currently supported.";
        break;
      }
    }
    if (channels > CV_CN_MAX) {
      LOG(FATAL) << "Error: num channels (" << channels
                 << ") exceeds CV_CN_MAX (" << CV_CN_MAX << ")";
    }
    for (decltype(batch_size) b = 0; b < batch_size; ++b) {
      cv::Mat temp(std::vector<int>({height, width}), CV_32FC(channels));
      std::copy_n(output_tensor.flat<float>().data() +
                      (b * (height * width * channels)),
                  height * width * channels, (float*)temp.data);
      return_vector.push_back(temp);
    }

    ret[output_layer_names[count++]] = return_vector;
  }
  return ret;
}
