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

// Multi-face tracking using face feature

#include "operator/face_tracker.h"

#include "common/context.h"

FaceTracker::FaceTracker(size_t rem_size)
    : Operator(OPERATOR_TYPE_FACE_TRACKER, {"input"}, {"output"}),
      rem_size_(rem_size),
      first_frame_(true) {}

std::shared_ptr<FaceTracker> FaceTracker::Create(const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

bool FaceTracker::Init() {
  LOG(INFO) << "FaceTracker initialized";
  return true;
}

bool FaceTracker::OnStop() { return true; }

void FaceTracker::Process() {
  auto frame = GetFrame("input");
  auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
  auto face_features =
      frame->GetValue<std::vector<std::vector<float>>>("face_features");
  CHECK(bboxes.size() == face_features.size());

  std::vector<PointFeature> point_features;
  for (size_t i = 0; i < bboxes.size(); i++) {
    cv::Point point(bboxes[i].px + bboxes[i].width / 2,
                    bboxes[i].py + bboxes[i].height / 2);
    point_features.push_back(PointFeature(point, face_features[i]));
  }

  if (first_frame_) {
    first_frame_ = false;
  } else {
    AttachNearest(point_features, 20.0);
  }
  for (const auto& m : point_features) {
    std::list<boost::optional<PointFeature>> l;
    l.push_back(m);
    path_list_.push_back(l);
  }

  for (auto it = path_list_.begin(); it != path_list_.end();) {
    if (it->size() > rem_size_) it->pop_front();

    bool list_all_empty_point = true;
    for (const auto& m : *it) {
      if (m) list_all_empty_point = false;
    }

    if (list_all_empty_point)
      path_list_.erase(it++);
    else
      it++;
  }

  PushFrame("output", std::move(frame));
}

void FaceTracker::AttachNearest(std::vector<PointFeature>& point_features,
                                float threshold) {
  for (auto& m : path_list_) {
    boost::optional<PointFeature> lp = m.back();
    if (!lp) {
      m.push_back(boost::optional<PointFeature>());
      continue;
    }

    auto it_result = point_features.end();
    float distance = std::numeric_limits<float>::max();
    for (auto it = point_features.begin(); it != point_features.end(); it++) {
      float d = GetDistance(lp->face_feature, it->face_feature);
      if ((d < distance) && (d < threshold)) {
        distance = d;
        it_result = it;
      }
    }

    if (it_result != point_features.end()) {
      m.push_back(*it_result);
      point_features.erase(it_result);
    } else {
      m.push_back(boost::optional<PointFeature>());
    }
  }
}

float FaceTracker::GetDistance(const std::vector<float>& a,
                               const std::vector<float>& b) {
  float distance = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    distance += pow((a[i] - b[i]), 2);
  }
  distance = sqrt(distance);

  return distance;
}
