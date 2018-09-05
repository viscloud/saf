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

// An example application to display camera data.

#include <cstdio>

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

void Run(const std::string& camera_name, float zoom, unsigned int angle,
         bool display) {
  std::shared_ptr<Camera> camera;
  CameraManager& camera_manager = CameraManager::GetInstance();

  CHECK(camera_manager.HasCamera(camera_name))
      << "Camera " << camera_name << " does not exist";

  camera = camera_manager.GetCamera(camera_name);
  camera->Start();

  std::cout << "Press \"Ctrl-C\" to stop." << std::endl;

  auto reader = camera->GetStream()->Subscribe();

  while (true) {
    auto frame = reader->PopFrame();
    if (frame != nullptr) {
      if (frame->IsStopFrame()) {
        break;
      }
      if (display && frame->Count("original_image")) {
        const cv::Mat& img = frame->GetValue<cv::Mat>("original_image");
        cv::Mat m;
        cv::resize(img, m, cv::Size(), zoom, zoom);
        RotateImage(m, angle);
        cv::imshow(camera_name, m);

        unsigned char q = cv::waitKey(10);
        if (q == 'q') break;
      }
      std::cout << frame->ToString() << std::endl;
    }
  }

  camera->Stop();
}

int main(int argc, char* argv[]) {
  // FIXME: Use more standard arg parse routine.
  // Set up glog
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;

  po::options_description desc("Simple camera display test");
  desc.add_options()("help,h", "print the help message");
  desc.add_options()("camera",
                     po::value<std::string>()->value_name("CAMERA")->required(),
                     "The name of the camera to use");
  desc.add_options()("config_dir,C",
                     po::value<std::string>()->value_name("CONFIG_DIR"),
                     "The directory to find SAF's configurations");
  desc.add_options()("rotate,r", po::value<int>()->default_value(0),
                     "Angle to rotate image; must be 0, 90, 180, or 270");
  desc.add_options()("zoom,z", po::value<float>()->default_value(1.0),
                     "Zoom factor by which to scale image for display");
  desc.add_options()("display,d", "Display the video stream.");

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

  ///////// Parse arguments
  if (vm.count("config_dir")) {
    Context::GetContext().SetConfigDir(vm["config_dir"].as<std::string>());
  }
  // Init SAF context, this must be called before using SAF.
  Context::GetContext().Init();

  auto camera_name = vm["camera"].as<std::string>();
  auto zoom = vm["zoom"].as<float>();

  auto angles = std::set<int>{0, 90, 180, 270};
  auto angle = vm["rotate"].as<int>();
  if (!angles.count(angle)) {
    std::cerr << "--rotate angle must be 0, 90, 180, or 270" << std::endl;
    std::cerr << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }
  bool display = vm.count("display");

  Run(camera_name, zoom, angle, display);

  return 0;
}
