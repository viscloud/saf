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

#include <gtest/gtest.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <caffe/caffe.hpp>
#include <opencv2/opencv.hpp>

#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

#include "camera/camera.h"
#include "common/serialization.h"
#include "common/types.h"
#include "model/model.h"
#include "model/model_manager.h"
#include "operator/neural_net_evaluator.h"
#include "stream/frame.h"
#include "stream/stream.h"

constexpr auto ALPHA = 0.001f;

constexpr auto CHANNELS = 3;
constexpr auto WIDTH = 224;
constexpr auto HEIGHT = 224;
const auto SHAPE = cv::Size(WIDTH, HEIGHT);

constexpr auto INPUT_IMAGE_FILEPATH = "data/input.jpg";
constexpr auto NETWORK_FILEPATH = "data/mobilenet/mobilenet_deploy.prototxt";
constexpr auto WEIGHTS_FILEPATH = "/tmp/mobilenet.caffemodel";

const std::vector<std::string> OUTPUTS = {
    "conv1",       "conv2_1/dw",  "conv2_1/sep", "conv2_2/dw",  "conv2_2/sep",
    "conv3_1/dw",  "conv3_1/sep", "conv3_2/dw",  "conv3_2/sep", "conv4_1/dw",
    "conv4_1/sep", "conv4_2/dw",  "conv4_2/sep", "conv5_1/dw",  "conv5_1/sep",
    "conv5_2/dw",  "conv5_2/sep", "conv5_3/dw",  "conv5_3/sep", "conv5_4/dw",
    "conv5_4/sep", "conv5_5/dw",  "conv5_5/sep", "conv5_6/dw",  "conv5_6/sep",
    "conv6/dw",    "conv6/sep",   "pool6",       "fc7",         "prob"};

bool FloatEqual(float lhs, float rhs) {
  if (lhs < 0) {
    lhs *= -1;
    rhs *= -1;
  }
  return lhs - lhs * ALPHA <= rhs && lhs + lhs * ALPHA >= rhs;
}

void CvMatEqual(cv::Mat lhs, cv::Mat rhs) {
  std::vector<int> cur_idx;
  if (lhs.dims != rhs.dims) {
    return;
  }
  CHECK_EQ(lhs.dims, rhs.dims);
  CHECK_GT(lhs.dims, 0);
  auto lhs_it = lhs.begin<float>();
  auto lhs_end = lhs.end<float>();
  auto rhs_it = rhs.begin<float>();
  auto rhs_end = rhs.end<float>();
  while (lhs_it != lhs_end && rhs_it != rhs_end) {
    CHECK(FloatEqual(*lhs_it, *rhs_it))
        << "Expects: " << *lhs_it << " Found: " << *rhs_it;
    ++lhs_it;
    ++rhs_it;
  }
}

cv::Mat Preprocess(const cv::Mat& img) {
  cv::Size input_geometry_ = cv::Size(WIDTH, HEIGHT);
  // Convert the input image to the input image format of the network.
  cv::Mat sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;
  return sample_resized;
}

TEST(TestNneCaffe, TestExtractIntermediateActivationsCaffe) {
  std::ifstream f(WEIGHTS_FILEPATH);
  ASSERT_TRUE(f.good()) << "The Caffe model file \"" << WEIGHTS_FILEPATH
                        << "\" was not found. Download it by executing: "
                        << "curl -o " << WEIGHTS_FILEPATH
                        << " https://raw.githubusercontent.com/cdwat/"
                           "MobileNet-Caffe/master/mobilenet.caffemodel";
  f.close();

  // Construct model
  Shape input_shape(CHANNELS, WIDTH, HEIGHT);
  ModelDesc desc("TestExtractIntermediateActivationsCaffe", MODEL_TYPE_CAFFE,
                 NETWORK_FILEPATH, WEIGHTS_FILEPATH, WIDTH, HEIGHT, "", "prob");
  NeuralNetEvaluator nne(desc, input_shape, 1, OUTPUTS);

  // Configure the mean image
  ModelManager::GetInstance().SetMeanColors(cv::Scalar(104.0, 117.0, 123.0));

  // Read the input
  cv::Mat original_image = cv::imread(INPUT_IMAGE_FILEPATH);
  ASSERT_FALSE(original_image.empty()) << "Image empty. Is a library (i.e. "
                                          "libtensorflow) clobbering libjpeg "
                                          "symbols?";
  cv::Mat preprocessed_image = Preprocess(original_image);

  // Construct frame with input image in it
  auto input_frame = std::make_unique<Frame>();
  input_frame->SetValue(Camera::kCaptureTimeMicrosKey,
                        boost::posix_time::microsec_clock::local_time());
  input_frame->SetValue("frame_id", (unsigned long)0);
  input_frame->SetValue("original_image", original_image);
  input_frame->SetValue("image", preprocessed_image);

  // Prepare stream
  StreamPtr stream = std::make_shared<Stream>();
  nne.SetSource("input", stream, "");

  // We need to set up the StreamReaders before calling Start().
  StreamReader* reader = nne.GetSink()->Subscribe();

  nne.Start();
  stream->PushFrame(std::move(input_frame));
  auto output_frame = reader->PopFrame();
  ASSERT_FALSE(output_frame == nullptr) << "Unable to get frame";
  for (decltype(OUTPUTS.size()) i = 0; i < OUTPUTS.size(); ++i) {
    std::string filename = OUTPUTS.at(i);
    boost::replace_all(filename, "/", ".");
    filename = "data/mobilenet/caffe_ground_truth/" + filename + ".bin";
    // contains the activations of the output layer
    std::ifstream gt_file;
    gt_file.open(filename);
    cv::Mat expected_output;
    boost::archive::binary_iarchive expected_output_archive(gt_file);

    try {
      expected_output_archive >> expected_output;
    } catch (std::exception& e) {
      LOG(INFO) << "Ignoring empty layer\n";
      continue;
    }

    gt_file.close();

    cv::Mat actual_output = output_frame->GetValue<cv::Mat>(OUTPUTS.at(i));
    int num_channel = actual_output.channels();
    int height = actual_output.rows;
    int width = actual_output.cols;
    float* gt_data_ptr = (float*)expected_output.ptr();
    int per_channel_floats = height * width;
    std::vector<cv::Mat> gt_channels;
    for (int i = 0; i < num_channel; ++i) {
      cv::Mat cur_channel(HEIGHT, WIDTH, CV_32F);
      memcpy(cur_channel.data, gt_data_ptr + per_channel_floats * i,
             per_channel_floats * sizeof(float));
      gt_channels.push_back(cur_channel);
    }
    cv::Mat expected_output_transposed;
    cv::merge(gt_channels, expected_output_transposed);
    CvMatEqual(expected_output_transposed, actual_output);
  }
  nne.Stop();
}
