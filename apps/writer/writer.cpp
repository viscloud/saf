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

void SignalHandler(int) {
  std::cout << "Received SIGINT, try to gracefully exit" << std::endl;
  exit(0);
}

void Run(bool display, const std::string& sender_endpoint,
         const std::string& sender_package_type,
         const std::string& write_target, const std::string& write_uri,
         const std::string& aux = "") {
  std::cout << "Run writer demo" << std::endl;
  std::signal(SIGINT, SignalHandler);

  // receiver
  auto receiver =
      std::make_shared<Receiver>(sender_endpoint, sender_package_type, aux);
  auto reader = receiver->GetSink(Receiver::GetSinkName())->Subscribe();
  auto writer = std::make_shared<Writer>(write_target, write_uri, 1);
  writer->SetSource(Writer::GetSourceName(0),
                    receiver->GetSink(Receiver::GetSinkName()));

  // Start the operators
  receiver->Start();
  writer->Start();

  if (display) {
    std::cout << "Press \"q\" to stop." << std::endl;
  } else {
    std::cout << "Press \"Control-C\" to stop." << std::endl;
  }

  //////// Operator started, display the results
  const std::vector<cv::Scalar> colors = GetColors(32);
  int color_count = 0;
  std::map<std::string, int> tags_colors;
  int fontface = cv::FONT_HERSHEY_SIMPLEX;
  double d_scale = 1;
  int thickness = 2;
  int baseline = 0;
  std::set<std::string> initialized_windows;
  while (true) {
    auto frame = reader->PopFrame();
    // Get Frame Rate
    double rate = reader->GetPushFps();
    std::ostringstream label;
    label.precision(2);
    label << rate << " FPS";
    auto label_string = label.str();
    if (display) {
      auto image = frame->GetValue<cv::Mat>("original_image");
      auto image_display = image.clone();
      auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
      if (frame->Count("face_landmarks") > 0) {
        auto face_landmarks =
            frame->GetValue<std::vector<FaceLandmark>>("face_landmarks");
        for (const auto& m : face_landmarks) {
          for (int j = 0; j < 5; j++)
            cv::circle(image_display, cv::Point(m.x[j], m.y[j]), 1,
                       cv::Scalar(255, 255, 0), 5);
        }
      }
      auto tags = frame->GetValue<std::vector<std::string>>("tags");
      for (size_t j = 0; j < tags.size(); ++j) {
        // Get the color
        int color_index;
        auto it = tags_colors.find(tags[j]);
        if (it == tags_colors.end()) {
          tags_colors.insert(std::make_pair(tags[j], color_count++));
          color_index = tags_colors.find(tags[j])->second;
        } else {
          color_index = it->second;
        }
        const cv::Scalar& color = colors[color_index];

        // Draw bboxes
        cv::Point top_left_pt(bboxes[j].px, bboxes[j].py);
        cv::Point bottom_right_pt(bboxes[j].px + bboxes[j].width,
                                  bboxes[j].py + bboxes[j].height);
        cv::rectangle(image_display, top_left_pt, bottom_right_pt, color, 4);
        cv::Point bottom_left_pt(bboxes[j].px, bboxes[j].py + bboxes[j].height);
        std::ostringstream text;
        text << tags[j];
        if (frame->Count("ids") > 0) {
          auto ids = frame->GetValue<std::vector<std::string>>("ids");
          std::size_t pos = ids[j].size();
          auto sheared_id = ids[j].substr(pos - 5);
          text << ": " << sheared_id;
        }
        cv::Size text_size = cv::getTextSize(text.str().c_str(), fontface,
                                             d_scale, thickness, &baseline);
        cv::rectangle(image_display, bottom_left_pt + cv::Point(0, 0),
                      bottom_left_pt + cv::Point(text_size.width,
                                                 -text_size.height - baseline),
                      color, CV_FILLED);
        cv::putText(image_display, text.str(),
                    bottom_left_pt - cv::Point(0, baseline), fontface, d_scale,
                    CV_RGB(0, 0, 0), thickness, 8);
      }

      // Overlay FPS
      auto font_scale = 2.0;
      cv::Point label_point(25, 50);
      cv::Scalar label_color(200, 200, 250);
      cv::Scalar outline_color(0, 0, 0);
      cv::putText(image_display, label_string, label_point,
                  CV_FONT_HERSHEY_PLAIN, font_scale, outline_color, 8, CV_AA);
      cv::putText(image_display, label_string, label_point,
                  CV_FONT_HERSHEY_PLAIN, font_scale, label_color, 2, CV_AA);

      auto camera_name = frame->GetValue<std::string>("camera_name");
      if (initialized_windows.count(camera_name) == 0) {
        cv::namedWindow(camera_name, cv::WINDOW_NORMAL);
        initialized_windows.insert(camera_name);
      }
      cv::imshow(camera_name, image_display);

      if (cv::waitKey(10) == 'q') break;
    }
  }

  LOG(INFO) << "Done";

  //////// Clean up
  reader->UnSubscribe();
  receiver->Stop();
  writer->Stop();
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
  desc.add_options()("sender_endpoint", po::value<std::string>()->required(),
                     "The remote endpoint address");
  desc.add_options()("sender_package_type",
                     po::value<std::string>()->default_value("frame"),
                     "The sender package type");
  desc.add_options()("write_target",
                     po::value<std::string>()->default_value(""),
                     "Writer target, e.g., file");
  desc.add_options()("write_uri", po::value<std::string>()->default_value(""),
                     "Writer uri, e.g., a filename");

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
  auto camera_name = vm["camera"].as<std::string>();
  auto sender_endpoint = vm["sender_endpoint"].as<std::string>();
  auto sender_package_type = vm["sender_package_type"].as<std::string>();
  auto write_target = vm["write_target"].as<std::string>();
  auto write_uri = vm["write_uri"].as<std::string>();
  Run(display, sender_endpoint, sender_package_type, write_target, write_uri,
      camera_name);

  return 0;
}
