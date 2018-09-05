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

#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <glog/logging.h>
#include <gst/gst.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

// Used to signal all threads that the pipeline should stop.
std::atomic<bool> stopped(false);

// Designed to be run by a separate thread. Waits for keyboard input, then
// notifies the application that it is time to stop.
void Stopper() {
  std::cout << "Press \"Enter\" to stop." << std::endl;
  getchar();
  stopped = true;
}

// Encodes the provided stream as a single H264 video.
void EncodeForever(StreamPtr stream, const std::string& field, int fps,
                   const std::string& filepath) {
  // Create GstVideoEncoder.
  auto encoder =
      std::make_shared<GstVideoEncoder>(field, filepath, -1, false, fps);
  encoder->SetSource(stream);
  encoder->Start();

  while (!stopped) {
    // Sleep for 1 second to keep overheads low.
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  encoder->Stop();
}

// Encodes the provided stream as a series of H264 videos.
void EncodeInterval(StreamPtr stream, const std::string& field, int fps,
                    const std::string& filepath,
                    boost::posix_time::time_duration reset_interval_s) {
  // Extract filepath components.
  size_t last_dot = filepath.find_last_of(".");
  std::string filepath_no_ext = filepath.substr(0, last_dot);
  std::string filepath_ext = filepath.substr(last_dot + 1, filepath.size());

  int file_count = 1;
  while (!stopped) {
    // Create a new filepath with an incremented suffix.
    std::ostringstream new_filepath;
    new_filepath << filepath_no_ext << "_" << file_count++ << "."
                 << filepath_ext;

    // Create GstVideoEncoder.
    auto encoder = std::make_shared<GstVideoEncoder>(field, new_filepath.str(),
                                                     -1, false, fps);
    encoder->SetSource(stream);
    encoder->Start();

    // Compute end time for this interval.
    boost::posix_time::ptime next_reset_s =
        boost::posix_time::second_clock::local_time() + reset_interval_s;
    while (!stopped &&
           (boost::posix_time::second_clock::local_time() < next_reset_s)) {
      // Sleep for 1 second because the granularity of the interval duration is
      // seconds.
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    encoder->Stop();
  }
}

// Designed to be run by a searate thread. Encodes the provided stream.
void Encoder(StreamPtr stream, const std::string& field, int fps,
             const std::string& filepath,
             boost::posix_time::time_duration reset_interval_s) {
  if (reset_interval_s == boost::posix_time::seconds(-1)) {
    EncodeForever(stream, field, fps, filepath);
  } else {
    EncodeInterval(stream, field, fps, filepath, reset_interval_s);
  }
}

void Run(bool use_camera, const std::string& camera_name,
         const std::string& publish_url, int fps, unsigned int angle,
         bool resize, int x_dim, int y_dim,
         boost::posix_time::time_duration reset_interval_s,
         const std::string& filepath) {
  std::vector<std::shared_ptr<Operator>> ops;

  StreamPtr stream;
  if (use_camera) {
    // Create Camera.
    std::shared_ptr<Camera> camera =
        CameraManager::GetInstance().GetCamera(camera_name);
    ops.push_back(camera);
    stream = camera->GetStream();
  } else {
    // Create FrameSubscriber.
    auto subscriber = std::make_shared<FrameSubscriber>(publish_url);
    ops.push_back(subscriber);
    stream = subscriber->GetSink();
  }

  std::string field;
  if (resize) {
    // Create ImageTransformer.
    auto transformer = std::make_shared<ImageTransformer>(
        Shape(3, x_dim, y_dim), false, angle);
    transformer->SetSource(stream);
    ops.push_back(transformer);
    stream = transformer->GetSink();

    // The ImageTransformer is hardcorded to store the resized image at the key
    // "image".
    field = "image";
  } else {
    field = "original_image";
  }

  // Launch stopper thread.
  std::thread stopper_thread([] { Stopper(); });

  // Launch the encoder thread. This is where the important logic resides.
  std::thread encoder_thread([stream, field, fps, filepath, reset_interval_s] {
    Encoder(stream, field, fps, filepath, reset_interval_s);
  });

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  // Wait for the helper threads to finish.
  encoder_thread.join();
  stopper_thread.join();

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }
}

int main(int argc, char* argv[]) {
  po::options_description desc("Stores a stream as an MP4 file.");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's config files.");
  desc.add_options()("camera,c", po::value<std::string>(),
                     "The name of the camera to use. Overrides "
                     "\"--publish-url\".");
  desc.add_options()("publish-url,u", po::value<std::string>(),
                     "The URL (host:port) on which the frame stream is being "
                     "published.");
  desc.add_options()("fps,f", po::value<int>()->default_value(30),
                     "The framerate at which to encode. Does not impact "
                     "playback rate.");
  desc.add_options()("rotate,r", po::value<int>()->default_value(0),
                     "The angle to rotate frames; must be 0, 90, 180, or 270.");
  desc.add_options()("x-dim,x", po::value<int>(),
                     "The width to which to resize the frames.");
  desc.add_options()("y-dim,y", po::value<int>(),
                     "The height to which to resize the frames.");
  desc.add_options()("reset-interval,i", po::value<long>(),
                     "Start a new output file as this interval (in seconds). "
                     "Leaving this out will create a single output file.");
  desc.add_options()("output-file,o", po::value<std::string>()->required(),
                     "The path to the output file.");

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

  std::string camera;
  bool use_camera = args.count("camera");
  if (use_camera) {
    camera = args["camera"].as<std::string>();
  }
  std::string publish_url;
  if (args.count("publish-url")) {
    publish_url = args["publish-url"].as<std::string>();
  } else if (!use_camera) {
    throw std::runtime_error(
        "Must specify either \"--camera\" or \"--publish-url\".");
  }
  auto fps = args["fps"].as<int>();
  auto angles = std::set<int>{0, 90, 180, 270};
  auto angle = args["rotate"].as<int>();
  if (angles.find(angle) == angles.end()) {
    std::ostringstream msg;
    msg << "Value for \"--rotate\" must be 0, 90, 180, or 270, but is: "
        << angle;
    throw std::runtime_error(msg.str());
  }
  bool resize = false;
  int x_dim = 0;
  if (args.count("x-dim")) {
    resize = true;
    x_dim = args["x-dim"].as<int>();
    if (x_dim < 1) {
      std::ostringstream msg;
      msg << "Value for \"--x-dim\" must be greater than 0, but is: " << x_dim;
      throw std::invalid_argument(msg.str());
    }
  }
  int y_dim = 0;
  if (args.count("y-dim")) {
    y_dim = args["y-dim"].as<int>();
    if (y_dim < 1) {
      std::ostringstream msg;
      msg << "Value for \"--y-dim\" must be greater than 0, but is: " << y_dim;
      throw std::invalid_argument(msg.str());
    }
    if (!resize) {
      throw std::invalid_argument(
          "\"--x-dim\" and \"--y-dim\" must be used together.");
    }
    resize = true;
  } else if (resize) {
    throw std::invalid_argument(
        "\"--x-dim\" and \"--y-dim\" must be used together.");
  }
  boost::posix_time::time_duration reset_interval_s =
      boost::posix_time::seconds(-1);
  if (args.count("reset-interval")) {
    reset_interval_s =
        boost::posix_time::seconds(args["reset-interval"].as<long>());
    if (reset_interval_s.is_negative()) {
      std::ostringstream msg;
      msg << "Value for \"--reset-interval\" cannot be negative, but is: "
          << reset_interval_s.total_seconds();
      throw std::invalid_argument(msg.str());
    }
  }
  auto filepath = args["output-file"].as<std::string>();
  Run(use_camera, camera, publish_url, fps, angle, resize, x_dim, y_dim,
      reset_interval_s, filepath);
  return 0;
}
