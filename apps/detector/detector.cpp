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

// An example application showing the usage of detector.

#include <csignal>

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;
using std::cout;
using std::endl;

/////// Global vars
std::vector<std::shared_ptr<Camera>> cameras;
std::vector<std::shared_ptr<Operator>> throttlers;
std::shared_ptr<ObjectDetector> object_detector;
std::vector<StreamReader*> detector_output_readers;
std::shared_ptr<Operator> sender;

void CleanUp() {
  if (sender && sender->IsStarted()) sender->Stop();

  for (auto reader : detector_output_readers) {
    reader->UnSubscribe();
  }

  if (object_detector != nullptr && object_detector->IsStarted())
    object_detector->Stop();

  for (auto throttler : throttlers) {
    if (throttler->IsStarted()) throttler->Stop();
  }

  for (auto camera : cameras) {
    if (camera->IsStarted()) camera->Stop();
  }
}

void SignalHandler(int) {
  std::cout << "Received SIGINT, try to gracefully exit" << std::endl;
  exit(0);
}

void Run(const std::vector<std::string>& camera_names,
         const std::string& detector_type, const std::string& detector_model,
         bool display, float detector_confidence_threshold,
         const std::string& detector_targets, int face_min_size,
         const std::string& sender_endpoint,
         const std::string& sender_package_type, int frames) {
  // Silence complier warning sayings when certain options are turned off.
  (void)detector_confidence_threshold;
  (void)detector_targets;

  cout << "Run detection demo" << endl;

  std::signal(SIGINT, SignalHandler);

  size_t batch_size = camera_names.size();
  CameraManager& camera_manager = CameraManager::GetInstance();
  ModelManager& model_manager = ModelManager::GetInstance();

  // Check options
  CHECK(model_manager.HasModel(detector_model))
      << "Model " << detector_model << " does not exist";
  for (auto camera_name : camera_names) {
    CHECK(camera_manager.HasCamera(camera_name))
        << "Camera " << camera_name << " does not exist";
  }

  ////// Start cameras, operators

  for (auto camera_name : camera_names) {
    auto camera = camera_manager.GetCamera(camera_name);
    cameras.push_back(camera);
  }

  // Throttlers
  for (size_t i = 0; i < batch_size; i++) {
    std::shared_ptr<Operator> throttler(new Throttler(30));
    throttler->SetSource("input", cameras[i]->GetSink("output"));
    throttlers.push_back(throttler);
  }

  // detector
  float detector_idle_duration = 0.f;
  auto model_descs = model_manager.GetModelDescs(detector_model);
  auto t = SplitString(detector_targets, ",");
  std::set<std::string> targets;
  for (const auto& m : t) {
    if (!m.empty()) targets.insert(m);
  }
  object_detector = std::make_shared<ObjectDetector>(
      detector_type, model_descs, batch_size, detector_confidence_threshold,
      detector_idle_duration, targets, face_min_size);
  for (size_t i = 0; i < batch_size; i++) {
    object_detector->SetSource("input" + std::to_string(i),
                               throttlers[i]->GetSink("output"));
  }

  // detector readers
  for (size_t i = 0; i < batch_size; i++) {
    auto detector_output =
        object_detector->GetSink("output" + std::to_string(i));
    detector_output_readers.push_back(detector_output->Subscribe());
  }

  // sender
  if (!sender_endpoint.empty()) {
    sender = std::make_shared<Sender>(sender_endpoint, sender_package_type,
                                      batch_size);
    for (size_t i = 0; i < batch_size; i++) {
      sender->SetSource(Sender::GetSourceName(i),
                        object_detector->GetSink("output" + std::to_string(i)));
    }
  }

  for (auto camera : cameras) {
    if (!camera->IsStarted()) {
      camera->Start();
    }
  }

  for (auto throttler : throttlers) {
    throttler->Start();
  }

  object_detector->Start();

  if (!sender_endpoint.empty()) {
    sender->Start();
  }

  //////// Operator started, display the results

  if (display) {
    for (std::string camera_name : camera_names) {
      cv::namedWindow(camera_name, cv::WINDOW_NORMAL);
    }
  }

  const std::vector<cv::Scalar> colors = GetColors(32);
  int color_count = 0;
  std::map<std::string, int> tags_colors;
  int fontface = cv::FONT_HERSHEY_SIMPLEX;
  double d_scale = 1;
  int thickness = 2;
  int baseline = 0;
  while (true) {
    for (size_t i = 0; i < camera_names.size(); i++) {
      auto reader = detector_output_readers[i];
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
          cv::Point bottom_left_pt(bboxes[j].px,
                                   bboxes[j].py + bboxes[j].height);
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
          cv::rectangle(
              image_display, bottom_left_pt + cv::Point(0, 0),
              bottom_left_pt +
                  cv::Point(text_size.width, -text_size.height - baseline),
              color, CV_FILLED);
          cv::putText(image_display, text.str(),
                      bottom_left_pt - cv::Point(0, baseline), fontface,
                      d_scale, CV_RGB(0, 0, 0), thickness, 8);
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

        cv::imshow(camera_names[i], image_display);
      }
    }

    if (display) {
      char q = cv::waitKey(10);
      if (q == 'q') break;
    }

    if (frames >= 0) {
      if (frames-- <= 0) break;
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
  desc.add_options()(
      "detector_type",
      po::value<std::string>()->value_name("DETECTOR_TYPE")->required(),
      "The name of the detector type to run");
  desc.add_options()(
      "detector_model,m",
      po::value<std::string>()->value_name("DETECTOR_MODEL")->required(),
      "The name of the detector model to run");
  desc.add_options()("detector_targets",
                     po::value<std::string>()->default_value(""),
                     "The name of the target to detect, separate with ,");
  desc.add_options()(
      "camera,c", po::value<std::string>()->value_name("CAMERAS")->required(),
      "The name of the camera to use, if there are multiple "
      "cameras to be used, separate with ,");
  desc.add_options()("display,d", "Enable display or not");
  desc.add_options()("device", po::value<int>()->default_value(-1),
                     "which device to use, -1 for CPU, > 0 for GPU device");
  desc.add_options()("config_dir,C",
                     po::value<std::string>()->value_name("CONFIG_DIR"),
                     "The directory to find SAF's configurations");
  desc.add_options()("detector_confidence_threshold",
                     po::value<float>()->default_value(0.5),
                     "detector confidence threshold");
  desc.add_options()("face_min_size", po::value<int>()->default_value(40),
                     "Face min size for mtcnn");
  desc.add_options()("sender_endpoint",
                     po::value<std::string>()->default_value(""),
                     "The remote endpoint address");
  desc.add_options()("sender_package_type",
                     po::value<std::string>()->default_value("thumbnails"),
                     "The sender package type");
  desc.add_options()("frames", po::value<int>()->default_value(-1),
                     "Total running frames");

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

  auto camera_names = SplitString(vm["camera"].as<std::string>(), ",");
  auto detector_type = vm["detector_type"].as<std::string>();
  auto detector_model = vm["detector_model"].as<std::string>();
  auto detector_targets = vm["detector_targets"].as<std::string>();
  bool display = vm.count("display") != 0;
  float detector_confidence_threshold =
      vm["detector_confidence_threshold"].as<float>();
  int face_min_size = vm["face_min_size"].as<int>();
  auto sender_endpoint = vm["sender_endpoint"].as<std::string>();
  auto sender_package_type = vm["sender_package_type"].as<std::string>();
  auto frames = vm["frames"].as<int>();
  Run(camera_names, detector_type, detector_model, display,
      detector_confidence_threshold, detector_targets, face_min_size,
      sender_endpoint, sender_package_type, frames);

  return 0;
}
