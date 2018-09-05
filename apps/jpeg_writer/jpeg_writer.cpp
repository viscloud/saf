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

// The JpegWriter is a simple app that reads frames from a single camera and
// immediately saves them as JPEG images.

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <gst/gst.h>
#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

void Run(const std::string& camera_name, bool resize, int x_dim, int y_dim,
         bool rotate, int angle, const std::string& field,
         const std::string& output_dir, bool organize_by_time,
         unsigned long frames_per_dir) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Create Camera.
  std::shared_ptr<Camera> camera =
      CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);

  StreamPtr stream = camera->GetStream();
  std::string field_to_save;
  if (resize || rotate) {
    // Create ImageTransformer. Set the target dimensions properly based on
    // whether we are resizing.
    int new_x_dim = 0;
    int new_y_dim = 0;
    if (resize) {
      new_x_dim = x_dim;
      new_y_dim = y_dim;
    } else {
      new_x_dim = camera->GetWidth();
      new_y_dim = camera->GetHeight();
    }
    auto transformer = std::make_shared<ImageTransformer>(
        Shape(3, new_x_dim, new_y_dim), false, angle);
    transformer->SetSource(camera->GetStream());
    ops.push_back(transformer);
    stream = transformer->GetSink();

    // The ImageTransformer stores its output using a custom key.
    field_to_save = ImageTransformer::kOutputKey;
  } else {
    field_to_save = field;
  }

  // Create JpegWriter.
  auto writer = std::make_shared<JpegWriter>(field_to_save, output_dir,
                                             organize_by_time, frames_per_dir);
  writer->SetSource(stream);
  ops.push_back(writer);

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  std::cout << "Press \"Enter\" to stop." << std::endl;
  getchar();

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }
}

int main(int argc, char* argv[]) {
  po::options_description desc("Stores frames as JPEG images");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's config files.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()("x-dim,x", po::value<int>(),
                     "The width to which to resize the frames. Forces "
                     "\"--field\" to be \"original_image\".");
  desc.add_options()("y-dim,y", po::value<int>(),
                     "The height to which to resize the frames. Forces "
                     "\"--field\" to be \"original_image\".");
  desc.add_options()("rotate,r", po::value<int>(),
                     "The angle to rotate frames; must be 0, 90, 180, or 270.");
  desc.add_options()("field,f",
                     po::value<std::string>()->default_value("original_image"),
                     "The field to save as a JPEG. Assumed to be "
                     "\"original_image\" when using \"--resize\".");
  desc.add_options()("output-dir,o", po::value<std::string>()->required(),
                     "The directory in which to store the frame JPEGs.");
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
  bool resize = false;
  int x_dim = 0;
  if (args.count("x-dim")) {
    resize = true;
    x_dim = args["x-dim"].as<int>();
    if (x_dim < 1) {
      std::ostringstream msg;
      msg << "Value for \"--x-dim\" must be greater than 0, but is: " << x_dim;
      throw std::runtime_error(msg.str());
    }
  }
  int y_dim = 0;
  if (args.count("y-dim")) {
    y_dim = args["y-dim"].as<int>();
    if (y_dim < 1) {
      std::ostringstream msg;
      msg << "Value for \"--y-dim\" must be greater than 0, but is: " << y_dim;
      throw std::runtime_error(msg.str());
    }
    if (!resize) {
      throw std::runtime_error(
          "\"--x-dim\" and \"--y-dim\" must be used together.");
    }
    resize = true;
  } else if (resize) {
    throw std::runtime_error(
        "\"--x-dim\" and \"--y-dim\" must be used together.");
  }
  bool rotate = args.count("rotate");
  int angle = 0;
  if (rotate) {
    auto angles = std::set<int>{0, 90, 180, 270};
    angle = args["rotate"].as<int>();
    if (angles.find(angle) == angles.end()) {
      std::ostringstream msg;
      msg << "Value for \"--rotate\" must be 0, 90, 180, or 270, but is: "
          << angle;
      throw std::runtime_error(msg.str());
    }
  }
  auto field = args["field"].as<std::string>();
  auto output_dir = args["output-dir"].as<std::string>();
  bool organize_by_time = args.count("organize-by-time");
  auto frames_per_dir = args["frames-per-dir"].as<unsigned long>();
  Run(camera, resize, x_dim, y_dim, rotate, angle, field, output_dir,
      organize_by_time, frames_per_dir);
  return 0;
}
