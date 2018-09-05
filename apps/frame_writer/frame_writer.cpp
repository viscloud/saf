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

// The frame_writer is a simple app that reads frames from a single camera and
// immediately saves several of their fields to disk as JSON files.

#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <glog/logging.h>
#include <gst/gst.h>
#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

void Run(const std::string& camera_name,
         const std::unordered_set<std::string> fields,
         const std::string& output_dir, bool use_binary,
         bool save_fields_separately, bool organize_by_time,
         unsigned long frames_per_dir) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Create Camera.
  std::shared_ptr<Camera> camera =
      CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);

  // Create FrameWriter.
  FrameWriter::FileFormat format;
  if (use_binary) {
    format = FrameWriter::FileFormat::BINARY;
  } else {
    format = FrameWriter::FileFormat::JSON;
  }
  auto writer = std::make_shared<FrameWriter>(fields, output_dir, format,
                                              save_fields_separately,
                                              organize_by_time, frames_per_dir);
  writer->SetSource(camera->GetStream());
  ops.push_back(writer);

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  std::cout << "Press \"Enter\" to stop" << std::endl;
  getchar();

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }
}

int main(int argc, char* argv[]) {
  std::vector<std::string> default_fields = {Camera::kCaptureTimeMicrosKey,
                                             "frame_id"};
  std::ostringstream default_fields_str;
  default_fields_str << "{ ";
  for (const auto& field : default_fields) {
    default_fields_str << "\"" << field << "\" ";
  }
  default_fields_str << "}";

  po::options_description desc("Stores frames as JSON files.");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's configuration files.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()(
      "fields,f",
      po::value<std::vector<std::string>>()
          ->multitoken()
          ->composing()
          ->default_value(default_fields, default_fields_str.str()),
      "The fields to save.");
  desc.add_options()("output-dir,o", po::value<std::string>()->required(),
                     "The directory in which to store the frame files.");
  desc.add_options()("use-binary",
                     "Whether to save the frame data as a binary file "
                     "(default: JSON)");
  desc.add_options()("save-fields-separately,s",
                     "Whether to save each field in a separate file.");
  desc.add_options()("organize-by-time,t",
                     "Whether to organize the output file by date and time.");
  desc.add_options()("frames-per-dir,n",
                     po::value<unsigned long>()->default_value(1000),
                     "The number of frames to save in each subdir. Only valid "
                     "if \"--organize-by-time\" is not specified.");

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

  // Extract the command line arguments.
  if (args.count("config-dir")) {
    Context::GetContext().SetConfigDir(args["config-dir"].as<std::string>());
  }
  // Initialize the SAF context. This must be called before using SAF.
  Context::GetContext().Init();

  auto camera = args["camera"].as<std::string>();
  auto fields = args["fields"].as<std::vector<std::string>>();
  auto output_dir = args["output-dir"].as<std::string>();
  bool use_binary = args.count("use-binary");
  bool save_fields_separately = args.count("save-fields-separately");
  bool organize_by_time = args.count("organize-by-time");
  auto frames_per_dir = args["frames-per-dir"].as<unsigned long>();
  Run(camera, std::unordered_set<std::string>{fields.begin(), fields.end()},
      output_dir, use_binary, save_fields_separately, organize_by_time,
      frames_per_dir);
  return 0;
}
