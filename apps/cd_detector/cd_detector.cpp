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

#include <csignal>

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;

/////// Global vars
std::vector<std::shared_ptr<Camera>> cameras;
std::vector<std::shared_ptr<Operator>> optical_flows;
std::vector<StreamReader*> output_readers;

void CleanUp() {
  for (auto reader : output_readers) {
    reader->UnSubscribe();
  }

  for (auto optical_flow : optical_flows) {
    if (optical_flow->IsStarted()) optical_flow->Stop();
  }

  for (auto camera : cameras) {
    if (camera->IsStarted()) camera->Stop();
  }
}

void SignalHandler(int) {
  std::cout << "Received SIGINT, try to gracefully exit" << std::endl;
  exit(0);
}

void DrawOptFlowMap(const cv::Mat& flow, cv::Mat& cflowmap, int step, double,
                    const cv::Scalar& color) {
  for (int y = 0; y < cflowmap.rows; y += step) {
    for (int x = 0; x < cflowmap.cols; x += step) {
      const cv::Point2f& fxy = flow.at<cv::Point2f>(y, x);
      cv::line(cflowmap, cv::Point(x, y),
               cv::Point(cvRound(x + fxy.x), cvRound(y + fxy.y)), color);
      cv::circle(cflowmap, cv::Point(x, y), 2, color, -1);
    }
  }
}

void Run(const std::vector<std::string>& camera_names, bool display) {
  std::cout << "Run cd_detector demo" << std::endl;
  std::signal(SIGINT, SignalHandler);

  size_t batch_size = camera_names.size();
  CameraManager& camera_manager = CameraManager::GetInstance();

  // Check options
  for (auto camera_name : camera_names) {
    CHECK(camera_manager.HasCamera(camera_name))
        << "Camera " << camera_name << " does not exist";
  }

  ////// Start cameras, operators

  for (auto camera_name : camera_names) {
    auto camera = camera_manager.GetCamera(camera_name);
    cameras.push_back(camera);
  }

  // optical_flows
  for (size_t i = 0; i < batch_size; i++) {
    auto optical_flow = std::make_shared<OpenCVOpticalFlow>();
    optical_flow->SetSource("input", cameras[i]->GetSink("output"));
    optical_flows.push_back(optical_flow);
  }

  // output_readers
  for (size_t i = 0; i < batch_size; i++) {
    output_readers.push_back(optical_flows[i]->GetSink("output")->Subscribe());
  }

  // Start the operators
  for (auto camera : cameras) {
    if (!camera->IsStarted()) {
      camera->Start();
    }
  }

  for (auto optical_flow : optical_flows) {
    optical_flow->Start();
  }

  if (display) {
    std::cout << "Press \"q\" to stop." << std::endl;
  } else {
    std::cout << "Press \"Control-C\" to stop." << std::endl;
  }

  //////// Operator started, display the results
  while (true) {
    for (size_t i = 0; i < camera_names.size(); i++) {
      auto reader = output_readers[i];
      auto frame = reader->PopFrame();
      // Get Frame Rate
      double rate = reader->GetPushFps();
      std::ostringstream label;
      label.precision(2);
      label << rate << " FPS";
      auto label_string = label.str();
      if (display) {
        auto cflow = frame->GetValue<cv::Mat>("cflow");
        auto flow = frame->GetValue<cv::Mat>("flow");
        DrawOptFlowMap(flow, cflow, 16, 1.5, cv::Scalar(0, 255, 0));

        // Overlay FPS
        auto font_scale = 2.0;
        cv::Point label_point(25, 50);
        cv::Scalar label_color(200, 200, 250);
        cv::Scalar outline_color(0, 0, 0);
        cv::putText(cflow, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                    font_scale, outline_color, 8, CV_AA);
        cv::putText(cflow, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                    font_scale, label_color, 2, CV_AA);
        cv::imshow(camera_names[i], cflow);
      }
    }

    if (display) {
      char q = cv::waitKey(10);
      if (q == 'q') break;
    }
  }

  LOG(INFO) << "Done";

  //////// Clean up
  CleanUp();
  cv::destroyAllWindows();
}

int main(int argc, char* argv[]) {
  // FIXME: Use more standard arg parse routine.
  // Set up glog
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;

  po::options_description desc("Multi-camera end to end video ingestion demo");
  desc.add_options()("help,h", "print the help message");
  desc.add_options()("display,d", "Enable display or not");
  desc.add_options()("device", po::value<int>()->default_value(-1),
                     "which device to use, -1 for CPU, > 0 for GPU device");
  desc.add_options()("config_dir,C", po::value<std::string>(),
                     "The directory to find SAF's configurations");
  desc.add_options()("camera,c", po::value<std::string>()->default_value(""),
                     "The name of the camera to use");

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
  int device_number = vm["device"].as<int>();
  Context::GetContext().SetInt(DEVICE_NUMBER, device_number);

  bool display = vm.count("display") != 0;
  auto camera_names = SplitString(vm["camera"].as<std::string>(), ",");
  Run(camera_names, display);

  return 0;
}
