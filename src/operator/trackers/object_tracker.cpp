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

// Multi-target tracking operators

#include "operator/trackers/object_tracker.h"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/context.h"
#include "operator/trackers/dlib_tracker.h"
#include "operator/trackers/kf_tracker.h"
#include "utils/cv_utils.h"

ObjectTracker::ObjectTracker(const std::string& type)
    : Operator(OPERATOR_TYPE_OBJECT_TRACKER, {"input"}, {"output"}),
      type_(type) {}

std::shared_ptr<ObjectTracker> ObjectTracker::Create(
    const FactoryParamsType& params) {
  auto type = params.at("type");
  return std::make_shared<ObjectTracker>(type);
}

bool ObjectTracker::Init() {
  LOG(INFO) << "ObjectTracker initialized";
  return true;
}

bool ObjectTracker::OnStop() {
  tracker_list_.clear();
  return true;
}

void ObjectTracker::Process() {
  Timer timer;
  timer.Start();

  auto frame = GetFrame("input");
  auto image = frame->GetValue<cv::Mat>("original_image");
  if (image.channels() == 3) {
    cv::cvtColor(image, gray_image_, cv::COLOR_BGR2GRAY);
  } else {
    gray_image_ = image;
  }

  std::vector<Rect> tracked_bboxes;
  std::vector<std::string> tracked_tags;
  std::vector<std::string> tracked_ids;
  std::vector<std::vector<double>> features;
  if (frame->Count("bounding_boxes") > 0) {
    auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    LOG(INFO) << "Got new MetadataFrame, bboxes size is " << bboxes.size()
              << ", current tracker size is " << tracker_list_.size();
    std::vector<Rect> untracked_bboxes = bboxes;
    auto untracked_tags = frame->GetValue<std::vector<std::string>>("tags");
    CHECK(untracked_bboxes.size() == untracked_tags.size());
    for (auto it = tracker_list_.begin(); it != tracker_list_.end();) {
      cv::Rect rt;
      bool on_track = (*it)->TrackGetPossibleBB(gray_image_, untracked_bboxes,
                                                untracked_tags, rt);
      if (on_track) {
        if (InsideImage(rt, gray_image_)) {
          tracked_bboxes.push_back(Rect(rt.x, rt.y, rt.width, rt.height));
          tracked_tags.push_back((*it)->GetTag());
          tracked_ids.push_back((*it)->GetId());
          features.push_back((*it)->GetBBFeature());
        }
        it++;
      } else {
        LOG(INFO) << "Remove tracker: " << rt;
        tracker_list_.erase(it++);
      }
    }

    CHECK(untracked_bboxes.size() == untracked_tags.size());
    for (size_t i = 0; i < untracked_bboxes.size(); ++i) {
      LOG(INFO) << "Create new tracker";
      int x = untracked_bboxes[i].px;
      int y = untracked_bboxes[i].py;
      int w = untracked_bboxes[i].width;
      int h = untracked_bboxes[i].height;
      CHECK((x >= 0) && (y >= 0) && (x + w <= gray_image_.cols) &&
            (y + h <= gray_image_.rows));
      cv::Rect bb(x, y, w, h);
      boost::uuids::uuid id = boost::uuids::random_generator()();
      std::string id_str = boost::lexical_cast<std::string>(id);
      std::shared_ptr<BaseTracker> new_tracker;
      if (type_ == "dlib") {
        new_tracker = std::make_shared<DlibTracker>(id_str, untracked_tags[i]);
      } else if (type_ == "kf") {
        new_tracker.reset(new KFTracker(id_str, untracked_tags[i]));
      } else {
        LOG(FATAL) << "Tracker type " << type_ << " not supported.";
      }
      new_tracker->Initialize(gray_image_, bb);
      CHECK(new_tracker->IsInitialized());
      new_tracker->Track(gray_image_);
      cv::Rect rt(new_tracker->GetBB());
      if (InsideImage(rt, gray_image_)) {
        tracked_bboxes.push_back(Rect(rt.x, rt.y, rt.width, rt.height));
        tracked_tags.push_back(untracked_tags[i]);
        tracked_ids.push_back(id_str);
        features.push_back(new_tracker->GetBBFeature());
      }
      tracker_list_.push_back(new_tracker);
    }
    last_calibration_time_ = std::chrono::system_clock::now();
  } else {
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = now - last_calibration_time_;
    if (diff.count() >= 5) {
      LOG(ERROR) << "No metadata frame received within 5 seconds"
                 << ", need calibration ......";
    }
    for (auto it = tracker_list_.begin(); it != tracker_list_.end(); ++it) {
      (*it)->Track(gray_image_);
      cv::Rect rt((*it)->GetBB());
      if (InsideImage(rt, gray_image_)) {
        tracked_bboxes.push_back(Rect(rt.x, rt.y, rt.width, rt.height));
        tracked_tags.push_back((*it)->GetTag());
        tracked_ids.push_back((*it)->GetId());
        features.push_back((*it)->GetBBFeature());
      }
    }
  }

  frame->SetValue("bounding_boxes", tracked_bboxes);
  frame->SetValue("tags", tracked_tags);
  frame->SetValue("ids", tracked_ids);
  frame->SetValue("features", features);
  PushFrame("output", std::move(frame));
  LOG(INFO) << "ObjectTracker took " << timer.ElapsedMSec() << " ms";
}
