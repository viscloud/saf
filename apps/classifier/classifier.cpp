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

// The classifier app demonstrates how to use an ImageClassifier operator.

#include <cstdio>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <gst/gst.h>
#include <boost/program_options.hpp>
#include <opencv2/opencv.hpp>

#include "saf.h"

namespace po = boost::program_options;

void Run(const std::string& camera_name, const std::string& model_name,
         size_t batch_size, bool display) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Camera
  auto camera = CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);

  // ImageTransformer
  auto model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(3, model_desc.GetInputWidth(), model_desc.GetInputHeight());
  auto transformer = std::make_shared<ImageTransformer>(input_shape, true);
  transformer->SetSource("input", camera->GetSink("output"));
  ops.push_back(transformer);

  // ImageClassifier
  auto classifier =
      std::make_shared<ImageClassifier>(model_desc, input_shape, 1, batch_size);
  classifier->SetSource("input", transformer->GetSink("output"));
  ops.push_back(classifier);

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  if (display) {
    std::cout << "Press \"q\" to stop." << std::endl;
  } else {
    std::cout << "Press \"Control-C\" to stop." << std::endl;
  }

  auto reader = classifier->GetSink("output")->Subscribe();
  while (true) {
    auto frame = reader->PopFrame();

    // Extract match percentage.
    auto probs = frame->GetValue<std::vector<double>>("probabilities");
    auto prob_percent = probs.front() * 100;

    // Extract tag.
    auto tags = frame->GetValue<std::vector<std::string>>("tags");
    auto tag = tags.front();
    std::regex re(".+? (.+)");
    std::smatch results;
    std::string tag_name;
    if (!std::regex_match(tag, results, re)) {
      tag_name = tag;
    } else {
      tag_name = results[1];
    }

    // Get Frame Rate
    double rate = reader->GetPushFps();

    std::ostringstream label;
    label.precision(2);
    label << rate << " FPS - " << prob_percent << "% - " << tag_name;
    auto label_string = label.str();
    std::cout << label_string << std::endl;

    // For debugging purposes only...
    std::ostringstream fps_msg;
    fps_msg.precision(3);
    fps_msg << "  GetPushFps: " << reader->GetPushFps() << std::endl
            << "  GetPopFps: " << reader->GetPopFps() << std::endl
            << "  GetHistoricalFps: " << reader->GetHistoricalFps() << std::endl
            << "  GetAvgProcessingLatencyMs->FPS: "
            << (1000 / classifier->GetAvgProcessingLatencyMs()) << std::endl
            << "  GetTrailingAvgProcessingLatencyMs->FPS: "
            << (1000 / classifier->GetTrailingAvgProcessingLatencyMs());
    std::cout << fps_msg.str() << std::endl;

    if (display) {
      // Overlay classification label and probability
      auto font_scale = 2.0;
      cv::Point label_point(25, 50);
      cv::Scalar label_color(200, 200, 250);
      cv::Scalar outline_color(0, 0, 0);

      auto img = frame->GetValue<cv::Mat>("original_image");
      cv::putText(img, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                  font_scale, outline_color, 8, CV_AA);
      cv::putText(img, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                  font_scale, label_color, 2, CV_AA);
      cv::imshow(camera_name, img);

      if (cv::waitKey(10) == 'q') break;
    }
  }

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }
}

int main(int argc, char* argv[]) {
  po::options_description desc("Runs image classification on a video stream");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's configuration files.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()("model,m", po::value<std::string>()->required(),
                     "The name of the model to evaluate.");
  desc.add_options()("batch-size,s", po::value<size_t>()->default_value(1),
                     "The batch size to use when evaluating the DNN.");
  desc.add_options()("display,d", "Enable display or not");

  // Parse the command line arguments.
  po::variables_map args;
  try {
    po::store(po::parse_command_line(argc, argv, desc), args);
    if (args.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
    po::notify(args);
  } catch (const po::error& e) {
    std::cerr << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  // Set up GStreamer.
  gst_init(&argc, &argv);
  // Set up glog.
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  // Initialize the SAF context. This must be called before using SAF.
  Context::GetContext().Init();

  // Extract the command line arguments.
  if (args.count("config-dir")) {
    Context::GetContext().SetConfigDir(args["config-dir"].as<std::string>());
  }
  auto camera_name = args["camera"].as<std::string>();
  auto model = args["model"].as<std::string>();
  auto batch_size = args["batch-size"].as<size_t>();
  bool display = args.count("display");
  Run(camera_name, model, batch_size, display);
  return 0;
}
