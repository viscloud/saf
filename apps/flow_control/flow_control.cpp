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

// The flow_control app is a simple example of using the FlowControlEntrance and
// FlowControlExit operators. The pipeline is:
//   Camera -> FlowControlEntrace -> Throttler -> FlowControlExit -> Throttler
//
// The presence of the first Throttler verifies that the Throttler releases
// tokens when dropping frames while under flow control. The presence of the
// second Throttler verifies that the Throttler does not release tokens when
// dropping frames while not under flow control.

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

void Run(const std::string& camera_name, unsigned int tokens) {
  std::vector<std::shared_ptr<Operator>> ops;

  // Camera
  auto camera = CameraManager::GetInstance().GetCamera(camera_name);
  ops.push_back(camera);

  // FlowControlEntrance
  auto entrance = std::make_shared<FlowControlEntrance>(tokens);
  entrance->SetSource(camera->GetStream());
  ops.push_back(entrance);

  // First Throttler
  auto throttler1 = std::make_shared<Throttler>(10);
  throttler1->SetSource("input", entrance->GetSink());
  ops.push_back(throttler1);

  // FlowControlExit
  auto exit = std::make_shared<FlowControlExit>();
  exit->SetSource(throttler1->GetSink("output"));
  ops.push_back(exit);

  // Second Throttler
  auto throttler2 = std::make_shared<Throttler>(5);
  throttler2->SetSource("input", exit->GetSink());
  ops.push_back(throttler2);

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
  po::options_description desc("Simple camera display test");
  desc.add_options()("help,h", "Print the help message.");
  desc.add_options()("config-dir,C", po::value<std::string>(),
                     "The directory containing SAF's configuration files.");
  desc.add_options()("camera,c", po::value<std::string>()->required(),
                     "The name of the camera to use.");
  desc.add_options()("tokens,t", po::value<int>()->default_value(50),
                     "The number of flow control tokens to issue.");

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
  std::string camera_name = args["camera"].as<std::string>();
  int tokens = args["tokens"].as<int>();
  if (tokens < 0) {
    std::cerr << "\"--tokens\" cannot be negative, but is: " << tokens
              << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  Run(camera_name, (unsigned int)tokens);
  return 0;
}
