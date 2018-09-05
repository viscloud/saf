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

// This application throttles a camera stream and publishes it on the network.

#include <atomic>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

// Whether the pipeline has been stopped.
std::atomic<bool> stopped(false);

void ProgressTracker(StreamPtr stream) {
  StreamReader* reader = stream->Subscribe();
  while (!stopped) {
    std::unique_ptr<Frame> frame = reader->PopFrame();
    if (frame != nullptr) {
      std::cout << "\rSent frame " << frame->GetValue<unsigned long>("frame_id")
                << " from time: "
                << frame->GetValue<boost::posix_time::ptime>(
                       Camera::kCaptureTimeMicrosKey);
      // This is required in order to make the console update as soon as the
      // above log is printed. Without this, the progress log will not update
      // smoothly.
      std::cout.flush();
    }
  }
  reader->UnSubscribe();
}

void Run(const std::string& camera_name, double fps, bool resize, int x_dim,
         int y_dim, bool rotate, int angle,
         std::unordered_set<std::string> fields_to_send,
         const std::string& publish_url) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Create Camera.
  std::shared_ptr<Camera> camera =
      CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);

  StreamPtr stream = camera->GetStream();
  if (fps) {
    // Create Throttler.
    auto throttler = std::make_shared<Throttler>(fps);
    throttler->SetSource(stream);
    ops.push_back(throttler);
    stream = throttler->GetSink();
  }
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
    transformer->SetSource(stream);
    ops.push_back(transformer);
    stream = transformer->GetSink();

    fields_to_send.insert(ImageTransformer::kOutputKey);
  }

  // Create FramePublisher.
  auto publisher =
      std::make_shared<FramePublisher>(publish_url, fields_to_send);
  publisher->SetSource(stream);
  ops.push_back(publisher);

  std::thread progress_thread =
      std::thread([stream] { ProgressTracker(stream); });

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

  // Signal the progress thread to stop.
  stopped = true;
  progress_thread.join();
}

int main(int argc, char* argv[]) {
  std::vector<std::string> default_fields = {
      "frame_id", Camera::kCaptureTimeMicrosKey, "original_image"};
  std::ostringstream default_fields_str;
  default_fields_str << "{ ";
  for (const auto& field : default_fields) {
    default_fields_str << "\"" << field << "\" ";
  }
  default_fields_str << "}";

  po::options_description desc("Publishes a frame stream on the network");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's config files.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()("fps,f", po::value<double>()->default_value(0),
                     ("The desired maximum rate of the published stream. The "
                      "actual rate may be less. An fps of 0 disables "
                      "throttling."));
  desc.add_options()("x-dim,x", po::value<int>(),
                     "The width to which to resize the frames. Forces "
                     "\"--field\" to be \"original_image\".");
  desc.add_options()("y-dim,y", po::value<int>(),
                     "The height to which to resize the frames. Forces "
                     "\"--field\" to be \"original_image\".");
  desc.add_options()("rotate,r", po::value<int>(),
                     "The angle to rotate frames; must be 0, 90, 180, or 270.");
  desc.add_options()(
      "fields-to-send",
      po::value<std::vector<std::string>>()
          ->multitoken()
          ->composing()
          ->default_value(default_fields, default_fields_str.str()),
      "The fields to publish.");
  desc.add_options()("publish-url,u",
                     po::value<std::string>()->default_value("127.0.0.1:5536"),
                     "The URL (host:port) on which to publish the frame "
                     "stream.");

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

  auto camera_name = args["camera"].as<std::string>();
  auto fps = args["fps"].as<double>();
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
  auto fields_to_send = args["fields-to-send"].as<std::vector<std::string>>();
  auto publish_url = args["publish-url"].as<std::string>();
  Run(camera_name, fps, resize, x_dim, y_dim, rotate, angle,
      std::unordered_set<std::string>{fields_to_send.begin(),
                                      fields_to_send.end()},
      publish_url);
  return 0;
}
