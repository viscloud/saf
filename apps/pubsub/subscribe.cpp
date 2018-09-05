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

// This application attaches to a published frame stream, stores the frames on
// disk, and displays them.

#include <atomic>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <opencv2/opencv.hpp>

#include "saf.h"

namespace po = boost::program_options;

constexpr auto FIELD_TO_DISPLAY = "original_image";

// Whether the pipeline has been stopped.
std::atomic<bool> stopped(false);

void ProgressTracker(StreamPtr stream) {
  StreamReader* reader = stream->Subscribe();
  while (!stopped) {
    std::unique_ptr<Frame> frame = reader->PopFrame();
    if (frame != nullptr) {
      std::cout << "\rReceived frame "
                << frame->GetValue<unsigned long>("frame_id") << " from time: "
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

void Run(const std::string& publish_url,
         std::unordered_set<std::string> fields_to_save,
         bool save_fields_separately, bool save_original_bytes, bool compress,
         bool save_jpegs, const std::string& output_dir, bool display,
         unsigned int angle, float zoom) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Create FrameSubscriber.
  auto subscriber = std::make_shared<FrameSubscriber>(publish_url);
  ops.push_back(subscriber);

  StreamPtr stream = subscriber->GetSink();
  if (!fields_to_save.empty()) {
    // Create FrameWriter for frame fields (i.e., metadata).
    auto frame_writer = std::make_shared<FrameWriter>(
        fields_to_save, output_dir, FrameWriter::FileFormat::JSON,
        save_fields_separately, true);
    frame_writer->SetSource(stream);
    ops.push_back(frame_writer);
  }

  if (save_original_bytes) {
    if (compress) {
      // Create Compressor.
      auto compressor =
          std::make_shared<Compressor>(Compressor::CompressionType::BZIP2);
      compressor->SetSource(stream);
      ops.push_back(compressor);
      stream = compressor->GetSink();
    }

    // Create FrameWriter for writing original demosaiced image.
    auto image_writer = std::make_shared<FrameWriter>(
        std::unordered_set<std::string>{"original_image"}, output_dir,
        FrameWriter::FileFormat::BINARY, save_fields_separately, true);
    image_writer->SetSource(stream);
    ops.push_back(image_writer);
  }

  if (save_jpegs) {
    // Create JpegWriter.
    auto jpeg_writer =
        std::make_shared<JpegWriter>("original_image", output_dir, true);
    jpeg_writer->SetSource(stream);
    ops.push_back(jpeg_writer);
  }

  std::thread progress_thread =
      std::thread([stream] { ProgressTracker(stream); });
  StreamReader* reader = subscriber->GetSink()->Subscribe();

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  std::cout << "Press \"q\" to stop." << std::endl;

  while (true) {
    // Pop frames and display them.
    auto frame = reader->PopFrame();

    // If the frames contain the result of a transformation, then assume that we
    // want to display that.
    std::string field;
    if (frame->Count(ImageTransformer::kOutputKey)) {
      field = ImageTransformer::kOutputKey;
    } else {
      field = FIELD_TO_DISPLAY;
    }

    if (display && (frame != nullptr)) {
      // Extract image and display it.
      const cv::Mat& img = frame->GetValue<cv::Mat>(field);
      cv::Mat img_resized;
      cv::resize(img, img_resized, cv::Size(), zoom, zoom);
      RotateImage(img_resized, angle);
      cv::imshow(field, img_resized);

      if (cv::waitKey(10) == 'q') break;
    }
  }
  reader->UnSubscribe();

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }

  // Signal the progress thread to stop.
  stopped = true;
  progress_thread.join();
}

int main(int argc, char* argv[]) {
  po::options_description desc("Subscribes to, saves, and displays a stream");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's config files.");
  desc.add_options()("publish-url,u",
                     po::value<std::string>()->default_value("127.0.0.1:5536"),
                     "The URL (host:port) on which the frame stream is being "
                     "published.");
  desc.add_options()("fields-to-save",
                     po::value<std::vector<std::string>>()
                         ->multitoken()
                         ->composing()
                         ->default_value({}, "None"),
                     "The fields to save.");
  desc.add_options()("save-fields-separately",
                     "Whether to save each frame field in a separate file.");
  desc.add_options()("save-original-bytes",
                     "Whether to save the uncompressed, demosaiced image.");
  desc.add_options()("compress,c",
                     "Whether to compress the \"original_bytes\" field.");
  desc.add_options()("save-jpegs",
                     "Whether to save a JPEG of the "
                     "\"original_image\" field.");
  desc.add_options()("output-dir,o", po::value<std::string>()->required(),
                     ("The root directory of the image storage database."));
  desc.add_options()("display,d", "Whether to display the frames.");
  desc.add_options()("rotate,r", po::value<unsigned int>()->default_value(0),
                     "Angle to rotate image for display; Must be 0, 90, 180, "
                     "or 270.");
  desc.add_options()("zoom,z", po::value<float>()->default_value(1.0),
                     "Zoom factor by which to scale image for display.");

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

  auto publish_url = args["publish-url"].as<std::string>();
  auto fields_to_save = args["fields-to-save"].as<std::vector<std::string>>();
  bool save_fields_separately = args.count("save-fields-separately");
  bool save_original_bytes = args.count("save-original-bytes");
  bool compress = args.count("compress");
  bool save_jpegs = args.count("save-jpegs");
  auto output_dir = args["output-dir"].as<std::string>();
  bool display = args.count("display");

  auto angles = std::set<unsigned int>{0, 90, 180, 270};
  auto angle = args["rotate"].as<unsigned int>();
  if (angles.find(angle) == angles.end()) {
    std::cerr << "Error: \"--rotate\" angle must be 0, 90, 180, or 270."
              << std::endl;
    std::cerr << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }
  auto zoom = args["zoom"].as<float>();

  Run(publish_url,
      std::unordered_set<std::string>{fields_to_save.begin(),
                                      fields_to_save.end()},
      save_fields_separately, save_original_bytes, compress, save_jpegs,
      output_dir, display, angle, zoom);
  return 0;
}
