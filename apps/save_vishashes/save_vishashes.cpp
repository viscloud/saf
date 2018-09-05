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

#include <atomic>
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

// Whether the pipeline has been stopped.
std::atomic<bool> stopped(false);

void ProgressTracker(StreamPtr stream) {
  StreamReader* reader = stream->Subscribe();
  while (!stopped) {
    std::unique_ptr<Frame> frame = reader->PopFrame();
    if (frame != nullptr) {
      std::cout << "\rSaved feature vector for frame "
                << frame->GetValue<unsigned long>("frame_id");
      // This is required in order to make the console update as soon as the
      // above log is printed. Without this, the progress log will not update
      // smoothly.
      std::cout.flush();
    }
  }
  reader->UnSubscribe();
}

void Run(bool block, const std::string& camera_name,
         const std::string& model_name, const std::string& layer,
         size_t nne_batch_size, const std::string& output_dir,
         unsigned long frames_per_dir, bool save_jpegs) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Create Camera.
  std::shared_ptr<Camera> camera =
      CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);
  StreamPtr camera_stream = camera->GetStream();

  if (save_jpegs) {
    // Create JpegWriter.
    auto jpeg_writer = std::make_shared<JpegWriter>(
        "original_image", output_dir, false, frames_per_dir);
    jpeg_writer->SetSource(camera_stream);
    jpeg_writer->SetBlockOnPush(block);
    ops.push_back(jpeg_writer);
  }

  // Create ImageTransformer.
  ModelDesc model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(3, model_desc.GetInputWidth(), model_desc.GetInputHeight());
  auto transformer = std::make_shared<ImageTransformer>(input_shape, true);
  transformer->SetSource(camera_stream);
  transformer->SetBlockOnPush(block);
  ops.push_back(transformer);

  // Create NeuralNetEvaluator.
  auto nne = std::make_shared<NeuralNetEvaluator>(
      model_desc, input_shape, nne_batch_size, std::vector<std::string>{layer});
  nne->SetSource(transformer->GetSink());
  nne->SetBlockOnPush(block);
  ops.push_back(nne);
  StreamPtr nne_stream = nne->GetSink();

  // Create FrameWriter.
  auto frame_writer = std::make_shared<FrameWriter>(
      std::unordered_set<std::string>{"frame_id", layer}, output_dir,
      FrameWriter::FileFormat::JSON, false, false, frames_per_dir);
  frame_writer->SetSource(nne_stream);
  frame_writer->SetBlockOnPush(block);
  ops.push_back(frame_writer);

  auto reader = nne->GetSink()->Subscribe();

  std::thread progress_thread =
      std::thread([nne_stream] { ProgressTracker(nne_stream); });

  // Start the operators in reverse order.
  for (auto ops_it = ops.rbegin(); ops_it != ops.rend(); ++ops_it) {
    (*ops_it)->Start();
  }

  // Loop until we receive a stop frame.
  while (true) {
    std::unique_ptr<Frame> frame = reader->PopFrame();
    if (frame != nullptr && frame->IsStopFrame()) {
      break;
    }
  }

  // Stop the operators in forward order.
  for (const auto& op : ops) {
    op->Stop();
  }

  // Signal the progress thread to stop.
  stopped = true;
  progress_thread.join();
}

int main(int argc, char* argv[]) {
  po::options_description desc(
      "Stores the feature vectors for a camera stream as JSON files");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's configuration "
                     "files.");
  desc.add_options()("block,b", "Whether to block when pushing frames.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()("model,m", po::value<std::string>()->required(),
                     "The name of the model to evaluate.");
  desc.add_options()("layer,l", po::value<std::string>()->required(),
                     "The name of the layer to extract.");
  desc.add_options()("nne-batch-size,s", po::value<size_t>()->default_value(1),
                     "The batch size to use for DNN inference.");
  desc.add_options()("output-dir,o", po::value<std::string>()->required(),
                     "The directory in which to store the JPEGs and vishash "
                     "file.");
  desc.add_options()("frames-per-dir,n",
                     po::value<unsigned long>()->default_value(1000),
                     "The number of frames to put in each output subdir.");
  desc.add_options()("save-jpegs,j", "Whether to save a JPEG of every frame.");

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

  bool block = args.count("block");
  auto camera_name = args["camera"].as<std::string>();
  auto model = args["model"].as<std::string>();
  auto layer = args["layer"].as<std::string>();
  auto nne_batch_size = args["nne-batch-size"].as<size_t>();
  auto output_dir = args["output-dir"].as<std::string>();
  auto frames_per_dir = args["frames-per-dir"].as<unsigned long>();
  bool save_jpegs = args.count("save-jpegs");
  Run(block, camera_name, model, layer, nne_batch_size, output_dir,
      frames_per_dir, save_jpegs);
  return 0;
}
