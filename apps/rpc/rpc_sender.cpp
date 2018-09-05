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

// Send frames over RPC

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

// Global arguments
struct Configurations {
  // The name of the camera to use
  std::string camera_name;
  // Address of the server to send frames
  std::string server;
  // How many seconds to send stream
  unsigned int duration;
} CONFIG;

/**
 * @brief Send frames ver RPC
 *
 * Pipeline:  Camera -> FrameSender
 */
void Run() {
  // Set up camera
  auto camera_name = CONFIG.camera_name;
  auto& camera_manager = CameraManager::GetInstance();
  CHECK(camera_manager.HasCamera(camera_name))
      << "Camera " << camera_name << " does not exist";
  auto camera = camera_manager.GetCamera(camera_name);

  // Set up FrameSender (sends frames over RPC)
  auto frame_sender = new FrameSender(CONFIG.server);
  frame_sender->SetSource(camera->GetStream());

  frame_sender->Start();
  camera->Start();

  std::this_thread::sleep_for(std::chrono::seconds(CONFIG.duration));

  camera->Stop();
  frame_sender->Stop();
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;

  po::options_description desc("Simple Frame Sender App for SAF");
  desc.add_options()("help,h", "print the help message");
  desc.add_options()("config_dir,C", po::value<std::string>(),
                     "The directory to find SAF's configuration");
  desc.add_options()("server_url,s", po::value<std::string>()->required(),
                     "host:port to connect to (e.g., example.com:4444)");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use");
  desc.add_options()("duration,d", po::value<unsigned int>()->default_value(5),
                     "How long to send stream in seconds");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const po::error& e) {
    std::cerr << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  //// Parse arguments

  if (vm.count("config_dir")) {
    Context::GetContext().SetConfigDir(vm["config_dir"].as<std::string>());
  }

  // Init SAF context, this must be called before using SAF.
  Context::GetContext().Init();

  CONFIG.server = vm["server_url"].as<std::string>();
  CONFIG.camera_name = vm["camera"].as<std::string>();
  CONFIG.duration = vm["duration"].as<unsigned int>();

  Run();
}
