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

#include <iostream>

#include "saf.h"

int main(int argc, char* argv[]) {
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  // Init SAF context, this must be called before usign SAF.
  Context::GetContext().Init();

  CameraManager& camera_manager = CameraManager::GetInstance();

  if (std::string(argv[1]) == "-h" || argc < 2) {
    std::cerr << "Usage: ./people CAMERA DISPLAY \n"
              << "\n"
              << " CAMERA: The name of the camera\n"
              << " DISPLAY: Enable preview or not (true)\n";
  }

  std::string camera_name = argv[1];
  std::string display = argv[2];
  bool display_on = (display == "true");

  auto camera = camera_manager.GetCamera(camera_name);
  auto camera_stream = camera->GetStream();

  ObjectDetector people_detector("opencv-people");

  people_detector.SetSource("input", camera->GetStream());
  camera->Start();
  people_detector.Start();

  auto output_stream = people_detector.GetSink("output");
  auto output_reader = output_stream->Subscribe();
  if (display_on) cv::namedWindow("Image", cv::WINDOW_NORMAL);
  while (true) {
    auto frame = output_reader->PopFrame();
    auto image = frame->GetValue<cv::Mat>("original_image");
    auto results = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    cv::Scalar box_color(255, 0, 0);
    for (auto result : results) {
      cv::Rect rect(result.px, result.py, result.width, result.height);
      cv::rectangle(image, rect, box_color, 2);
    }
    if (display_on) cv::imshow("Image", image);
    int q = cv::waitKey(10);
    if (q == 'q') {
      break;
    }
  }
}
