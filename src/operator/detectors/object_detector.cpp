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

// Multi-target detection using FRCNN

#include "operator/detectors/object_detector.h"

#include "common/context.h"
#include "model/model_manager.h"
#ifdef USE_CAFFE
#include "operator/detectors/caffe_mtcnn_face_detector.h"
#include "operator/detectors/caffe_yolo_detector.h"
#endif  // USE_CAFFE
#ifdef HAVE_INTEL_CAFFE
#include "operator/detectors/caffe_yolo_v2_detector.h"
#ifdef USE_SSD
#include "operator/detectors/caffe_mobilenet_ssd_detector.h"
#include "operator/detectors/ssd_detector.h"
#endif  // USE_SSD
#endif  // HAVE_INTEL_CAFFE
#ifdef USE_CVSDK
#include "operator/detectors/cvsdk_ssd_detector.h"
#endif  // USE_CVSDK
#ifdef USE_FRCNN
#include "operator/detectors/frcnn_detector.h"
#endif  // USE_FRCNN
#ifdef USE_NCS
#include "operator/detectors/ncs_yolo_detector.h"
#endif  // USE_NCS
#include "operator/detectors/opencv_face_detector.h"
#include "operator/detectors/opencv_people_detector.h"
#include "utils/string_utils.h"

#define GET_SOURCE_NAME(i) ("input" + std::to_string(i))
#define GET_SINK_NAME(i) ("output" + std::to_string(i))

ObjectDetector::ObjectDetector(const std::string& type,
                               const std::vector<ModelDesc>& model_descs,
                               size_t batch_size, float confidence_threshold,
                               float idle_duration,
                               const std::set<std::string>& targets,
                               int face_min_size)
    : Operator(OPERATOR_TYPE_OBJECT_DETECTOR, {}, {}),
      type_(type),
      model_descs_(model_descs),
      batch_size_(batch_size),
      confidence_threshold_(confidence_threshold),
      idle_duration_(idle_duration),
      last_detect_time_(batch_size),
      targets_(targets),
      face_min_size_(face_min_size) {
  // Silence unused variable warning when compiling with minimal options.
  (void)face_min_size_;

  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({GET_SOURCE_NAME(i), nullptr});
    sinks_.insert({GET_SINK_NAME(i), std::shared_ptr<Stream>(new Stream)});
  }
}

std::shared_ptr<ObjectDetector> ObjectDetector::Create(
    const FactoryParamsType& params) {
  auto type = params.at("type");

  ModelManager& model_manager = ModelManager::GetInstance();
  auto model_name = params.at("model");
  CHECK(model_manager.HasModel(model_name));
  auto model_descs = model_manager.GetModelDescs(model_name);

  auto batch_size = StringToSizet(params.at("batch_size"));
  auto confidence_threshold = atof(params.at("confidence_threshold").c_str());
  auto idle_duration = atof(params.at("idle_duration").c_str());

  auto detector_targets = params.at("targets");
  auto t = SplitString(detector_targets, ",");
  std::set<std::string> targets;
  for (const auto& m : t) {
    if (!m.empty()) targets.insert(m);
  }

  auto face_min_size = StringToInt(params.at("face_min_size"));

  return std::make_shared<ObjectDetector>(type, model_descs, batch_size,
                                          confidence_threshold, idle_duration,
                                          targets, face_min_size);
}

bool ObjectDetector::Init() {
  bool result = false;
  if (type_ == "opencv-face") {
    detector_ = std::make_unique<OpenCVFaceDetector>(model_descs_.at(0));
    result = detector_->Init();
  } else if (type_ == "opencv-people") {
    detector_ = std::make_unique<OpenCVPeopleDetector>();
    result = detector_->Init();
#ifdef USE_CAFFE
  } else if (type_ == "mtcnn-face") {
    detector_ =
        std::make_unique<MtcnnFaceDetector>(model_descs_, face_min_size_);
    result = detector_->Init();
  } else if (type_ == "yolo") {
    detector_ = std::make_unique<YoloDetector>(model_descs_.at(0));
    result = detector_->Init();
#ifdef USE_ISAAC
  } else if (type_ == "yolo-v2-fp16") {
    detector_ = std::make_unique<YoloV2Detector<half>>(model_descs_.at(0));
    result = detector_->Init();
#endif  // USE_ISAAC
#ifdef USE_ISAAC
  } else if (type_ == "mobilenet-ssd-fp16") {
    detector_ = std::make_unique<MobilenetSsdDetector<half>>(model_descs_.at(0)));
    result = detector_->Init();
#endif  // USE_ISAAC
#endif  // USE_CAFFE
#ifdef HAVE_INTEL_CAFFE
  } else if (type_ == "yolo-v2") {
    detector_ = std::make_unique<YoloV2Detector<float>>(model_descs_.at(0));
    result = detector_->Init();
#ifdef USE_SSD
  } else if (type_ == "mobilenet-ssd") {
    detector_ =
        std::make_unique<MobilenetSsdDetector<float>>(model_descs_.at(0));
    result = detector_->Init();
  } else if (type_ == "ssd") {
    detector_ = std::make_unique<SsdDetector>(model_descs_.at(0));
    result = detector_->Init();
#endif  // USE_SSD
#endif  // HAVE_INTEL_CAFFE
#ifdef USE_FRCNN
  } else if (type_ == "frcnn") {
    detector_ = std::make_unique<FrcnnDetector>(model_descs_.at(0));
    result = detector_->Init();
#endif  // USE_FRCNN
#ifdef USE_NCS
  } else if (type_ == "ncs-yolo") {
    detector_ = std::make_unique<NcsYoloDetector>(model_descs_.at(0));
    result = detector_->Init();
#endif  // USE_NCS
#ifdef USE_CVSDK
#ifdef USE_SSD
  } else if (type_ == "cvsdk-ssd") {
    detector_ = std::make_unique<CVSDKSsdDetector>(model_descs_.at(0));
    result = detector_->Init();
#endif  // USE_SSD
#endif  // USE_CVSDK
  } else {
    LOG(FATAL) << "Detector type " << type_ << " not supported.";
  }

  return result;
}

bool ObjectDetector::OnStop() { return true; }

void ObjectDetector::Process() {
  Timer timer;
  timer.Start();

  std::unique_ptr<Frame> frame;
  for (size_t i = 0; i < batch_size_; i++) {
    frame = GetFrame(GET_SOURCE_NAME(i));
    if (!frame) continue;

    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = now - last_detect_time_[i];
    if (diff.count() >= idle_duration_) {
      auto original_img = frame->GetValue<cv::Mat>("original_image");
      CHECK(!original_img.empty());
      auto result = detector_->Detect(original_img);
      std::vector<ObjectInfo> filtered_res;
      for (const auto& m : result) {
        if (m.confidence > confidence_threshold_) {
          if (targets_.empty()) {
            filtered_res.push_back(m);
          } else {
            auto it = targets_.find(m.tag);
            if (it != targets_.end()) filtered_res.push_back(m);
          }
        }
      }

      std::vector<std::string> tags;
      std::vector<Rect> bboxes;
      std::vector<float> confidences;
      std::vector<FaceLandmark> face_landmarks;
      bool face_landmarks_flag = false;
      for (const auto& m : filtered_res) {
        tags.push_back(m.tag);
        cv::Rect cr = m.bbox;
        int x = cr.x;
        int y = cr.y;
        int w = cr.width;
        int h = cr.height;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if ((x + w) > original_img.cols) w = original_img.cols - x;
        if ((y + h) > original_img.rows) h = original_img.rows - y;
        bboxes.push_back(Rect(x, y, w, h));
        confidences.push_back(m.confidence);

        if (m.face_landmark_flag) {
          face_landmarks.push_back(m.face_landmark);
          face_landmarks_flag = true;
        }
      }

      last_detect_time_[i] = std::chrono::system_clock::now();
      frame->SetValue("tags", tags);
      frame->SetValue("bounding_boxes", bboxes);
      frame->SetValue("confidences", confidences);
      if (face_landmarks_flag)
        frame->SetValue("face_landmarks", face_landmarks);
      PushFrame(GET_SINK_NAME(i), std::move(frame));
      LOG(INFO) << "Object detection took " << timer.ElapsedMSec() << " ms";
    } else {
      PushFrame(GET_SINK_NAME(i), std::move(frame));
    }
  }
}

void ObjectDetector::SetInputStream(int src_id, StreamPtr stream) {
  SetSource(GET_SOURCE_NAME(src_id), stream);
}
