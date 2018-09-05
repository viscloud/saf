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

// An example application showing the usage of facenet tracker.

#include <csignal>

#include <boost/program_options.hpp>

#include "saf.h"

namespace po = boost::program_options;
using std::cout;
using std::endl;

/////// Global vars
std::vector<std::shared_ptr<Camera>> cameras;
std::vector<std::shared_ptr<Operator>> transformers;
std::vector<std::shared_ptr<Operator>> motion_detectors;
std::shared_ptr<ObjectDetector> object_detector;
std::shared_ptr<Facenet> facenet;
std::vector<std::shared_ptr<Operator>> trackers;
std::vector<StreamReader*> tracker_output_readers;
std::vector<std::shared_ptr<GstVideoEncoder>> encoders;

void CleanUp() {
  for (auto encoder : encoders) {
    if (encoder->IsStarted()) encoder->Stop();
  }

  for (auto reader : tracker_output_readers) {
    reader->UnSubscribe();
  }

  for (auto tracker : trackers) {
    if (tracker->IsStarted()) tracker->Stop();
  }

  if (facenet != nullptr && facenet->IsStarted()) facenet->Stop();

  if (object_detector != nullptr && object_detector->IsStarted())
    object_detector->Stop();

  for (auto motion_detector : motion_detectors) {
    if (motion_detector->IsStarted()) motion_detector->Stop();
  }

  for (auto transformer : transformers) {
    if (transformer->IsStarted()) transformer->Stop();
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
         const std::string& mtcnn_model_name,
         const std::string& facenet_model_name, bool display, float scale,
         float motion_threshold, float motion_max_duration) {
  // Silence complier warning sayings when certain options are turned off.
  (void)motion_threshold;
  (void)motion_max_duration;

  cout << "Run face_detector demo" << endl;

  std::signal(SIGINT, SignalHandler);

  size_t batch_size = camera_names.size();
  CameraManager& camera_manager = CameraManager::GetInstance();
  ModelManager& model_manager = ModelManager::GetInstance();

  // Check options
  CHECK(model_manager.HasModel(mtcnn_model_name))
      << "Model " << mtcnn_model_name << " does not exist";
  CHECK(model_manager.HasModel(facenet_model_name))
      << "Model " << facenet_model_name << " does not exist";
  for (auto camera_name : camera_names) {
    CHECK(camera_manager.HasCamera(camera_name))
        << "Camera " << camera_name << " does not exist";
  }

  ////// Start cameras, operators

  for (auto camera_name : camera_names) {
    auto camera = camera_manager.GetCamera(camera_name);
    cameras.push_back(camera);
  }

  // Do video stream classification
  std::vector<std::shared_ptr<Stream>> camera_streams;
  for (auto camera : cameras) {
    auto camera_stream = camera->GetStream();
    camera_streams.push_back(camera_stream);
  }
  Shape input_shape(3, cameras[0]->GetWidth() * scale,
                    cameras[0]->GetHeight() * scale);
  std::vector<std::shared_ptr<Stream>> input_streams;

  // Transformers
  for (auto camera_stream : camera_streams) {
    std::shared_ptr<Operator> transform_op(
        new ImageTransformer(input_shape, false));
    transform_op->SetSource("input", camera_stream);
    transformers.push_back(transform_op);
    input_streams.push_back(transform_op->GetSink("output"));
  }
  // mtcnn
  auto model_descs = model_manager.GetModelDescs(mtcnn_model_name);
  object_detector =
      std::make_unique<ObjectDetector>("mtcnn-face", model_descs, batch_size,
                                       0.f, 0.f, std::set<std::string>(), 40);
  for (size_t i = 0; i < batch_size; i++) {
    object_detector->SetInputStream(i, input_streams[i]);
  }

  // facenet
  auto model_desc = model_manager.GetModelDesc(facenet_model_name);
  Shape input_shape_facenet(3, model_desc.GetInputWidth(),
                            model_desc.GetInputHeight());
  facenet = std::make_shared<Facenet>(model_desc, input_shape_facenet,
                                      input_streams.size());

  for (size_t i = 0; i < batch_size; i++) {
    facenet->SetInputStream(
        i, object_detector->GetSink("output" + std::to_string(i)));
  }

  // tracker
  for (size_t i = 0; i < batch_size; i++) {
    std::shared_ptr<Operator> tracker(new FaceTracker());
    tracker->SetSource("input", facenet->GetSink("output" + std::to_string(i)));
    trackers.push_back(tracker);

    // tracker readers
    auto tracker_output = tracker->GetSink("output");
    tracker_output_readers.push_back(tracker_output->Subscribe());

    // encoders, encode each camera stream
    std::string output_filename = camera_names[i] + ".mp4";

    std::shared_ptr<GstVideoEncoder> encoder(
        new GstVideoEncoder("original_image", output_filename));
    encoder->SetSource("input", tracker->GetSink("output"));
    encoders.push_back(encoder);
  }

  for (auto camera : cameras) {
    if (!camera->IsStarted()) {
      camera->Start();
    }
  }

  for (auto transformer : transformers) {
    transformer->Start();
  }

  for (auto motion_detector : motion_detectors) {
    motion_detector->Start();
  }

  object_detector->Start();

  facenet->Start();

  for (auto tracker : trackers) {
    tracker->Start();
  }

  for (auto encoder : encoders) {
    encoder->Start();
  }

  //////// Operator started, display the results

  if (display) {
    for (std::string camera_name : camera_names) {
      cv::namedWindow(camera_name, cv::WINDOW_NORMAL);
    }
  }

  while (true) {
    for (size_t i = 0; i < camera_names.size(); i++) {
      auto reader = tracker_output_readers[i];
      auto frame = reader->PopFrame();
      if (display) {
        auto image = frame->GetValue<cv::Mat>("original_image");
        auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
        for (const auto& m : bboxes) {
          cv::rectangle(image, cv::Rect(m.px, m.py, m.width, m.height),
                        cv::Scalar(255, 0, 0), 5);
        }
        auto face_landmarks =
            frame->GetValue<std::vector<FaceLandmark>>("face_landmarks");
        for (const auto& m : face_landmarks) {
          for (int j = 0; j < 5; j++)
            cv::circle(image, cv::Point(m.x[j], m.y[j]), 1,
                       cv::Scalar(255, 255, 0), 5);
        }
        auto tags = frame->GetValue<std::vector<std::string>>("tags");
        for (size_t j = 0; j < tags.size(); ++j) {
          std::ostringstream text;
          if (frame->Count("confidences") != 0) {
            auto confidences =
                frame->GetValue<std::vector<float>>("confidences");
            text << tags[j] << "  :  " << confidences[j];
          } else
            text << tags[j];
          cv::putText(image, text.str(),
                      cv::Point(bboxes[j].px, bboxes[j].py + 30), 0, 1.0,
                      cv::Scalar(0, 255, 0), 3);
        }

        cv::imshow(camera_names[i], image);
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
  desc.add_options()(
      "mtcnn_model,m",
      po::value<std::string>()->value_name("MTCNN_MODEL")->required(),
      "The name of the mtcnn model to run");
  desc.add_options()(
      "facenet_model",
      po::value<std::string>()->value_name("FACENET_MODEL")->required(),
      "The name of the facenet model to run");
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
  desc.add_options()("scale,s", po::value<float>()->default_value(1.0),
                     "scale factor before mtcnn");
  desc.add_options()("motion_threshold", po::value<float>()->default_value(0.5),
                     "motion threshold");
  desc.add_options()("motion_max_duration",
                     po::value<float>()->default_value(1.0),
                     "motion max duration");

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
  auto mtcnn_model = vm["mtcnn_model"].as<std::string>();
  auto facenet_model = vm["facenet_model"].as<std::string>();
  bool display = vm.count("display") != 0;
  float scale = vm["scale"].as<float>();
  float motion_threshold = vm["motion_threshold"].as<float>();
  float motion_max_duration = vm["motion_max_duration"].as<float>();
  Run(camera_names, mtcnn_model, facenet_model, display, scale,
      motion_threshold, motion_max_duration);

  return 0;
}
