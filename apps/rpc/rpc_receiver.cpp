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

// Receive frames over RPC

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

/**
 * @brief Receive frames over RPC and display them
 *
 * Pipeline:  FrameReceiver -> Display
 */
void Run(std::string& server, float zoom, unsigned int angle) {
  // Set up FrameReceiver (receives frames over RPC)
  auto frame_receiver = new FrameReceiver(server);
  auto reader = frame_receiver->GetSink()->Subscribe();

  // Set up OpenCV display window
  const auto window_name = "output";
  cv::namedWindow(window_name);
  frame_receiver->Start();

  std::cout << "Press \"q\" to stop." << std::endl;

  while (true) {
    auto frame = reader->PopFrame(30);
    if (frame != nullptr) {
      const cv::Mat& img = frame->GetValue<cv::Mat>("original_image");
      cv::Mat m;
      cv::resize(img, m, cv::Size(), zoom, zoom);
      RotateImage(m, angle);
      cv::imshow(window_name, m);
    }

    unsigned char q = cv::waitKey(10);
    if (q == 'q') break;
  }

  frame_receiver->Stop();
  cv::destroyAllWindows();
}

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;

  po::options_description desc("Simple Frame Receiver App for SAF");
  desc.add_options()("help,h", "print the help message");
  desc.add_options()("config_dir,C", po::value<std::string>(),
                     "The directory to find SAF's configuration");
  desc.add_options()("rotate,r", po::value<unsigned int>()->default_value(0),
                     "Angle to rotate image; must be 0, 90, 180, or 270");
  desc.add_options()("zoom,z", po::value<float>()->default_value(1.0),
                     "Zoom factor by which to scale image for display");
  desc.add_options()("listen_url,l", po::value<std::string>()->required(),
                     "address:port to listen on (e.g., 0.0.0.0:4444)");

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

  auto server = vm["listen_url"].as<std::string>();
  auto zoom = vm["zoom"].as<float>();

  auto angles = std::set<unsigned int>{0, 90, 180, 270};
  auto angle = vm["rotate"].as<unsigned int>();
  if (!angles.count(angle)) {
    std::cerr << "--rotate angle must be 0, 90, 180, or 270" << std::endl;
    std::cerr << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  Run(server, zoom, angle);
}
