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

#ifndef SAF_OPERATOR_TRACKERS_OBJECT_TRACKER_H_
#define SAF_OPERATOR_TRACKERS_OBJECT_TRACKER_H_

#include <cv.h>

#include "operator/operator.h"

class BaseTracker {
 public:
  BaseTracker(const std::string& id, const std::string& tag)
      : id_(id), tag_(tag) {}
  virtual ~BaseTracker() {}
  std::string GetId() const { return id_; }
  std::string GetTag() const { return tag_; }
  virtual void Initialize(const cv::Mat& gray_image, cv::Rect bb) = 0;
  virtual bool IsInitialized() = 0;
  virtual void Track(const cv::Mat& gray_image) = 0;
  virtual cv::Rect GetBB() = 0;
  virtual std::vector<double> GetBBFeature() = 0;
  virtual bool OnTrack(const cv::Rect& ru, const cv::Rect& rt) {
    cv::Rect intersects = rt & ru;
    double intersects_percent = (double)intersects.area() / (double)ru.area();

    double area_diff = (double)abs(rt.area() - ru.area()) / (double)ru.area();
    return (intersects_percent >= 0.7) && (area_diff <= 0.3);
  }
  virtual bool TrackGetPossibleBB(const cv::Mat& gray_image,
                                  std::vector<Rect>& untracked_bboxes,
                                  std::vector<std::string>& untracked_tags,
                                  cv::Rect& rt) {
    Track(gray_image);
    rt = GetBB();

    bool on_track = false;
    for (size_t i = 0; i < untracked_bboxes.size(); ++i) {
      cv::Rect ru(untracked_bboxes[i].px, untracked_bboxes[i].py,
                  untracked_bboxes[i].width, untracked_bboxes[i].height);
      if (OnTrack(ru, rt)) {
        untracked_bboxes.erase(untracked_bboxes.begin() + i);
        untracked_tags.erase(untracked_tags.begin() + i);
        on_track = true;
        break;
      }
    }
    return on_track;
  }

 private:
  std::string id_;
  std::string tag_;
};

class ObjectTracker : public Operator {
 public:
  ObjectTracker(const std::string& type);
  static std::shared_ptr<ObjectTracker> Create(const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string type_;
  std::list<std::shared_ptr<BaseTracker>> tracker_list_;
  cv::Mat gray_image_;
  std::chrono::time_point<std::chrono::system_clock> last_calibration_time_;
};

#endif  // SAF_OPERATOR_TRACKERS_OBJECT_TRACKER_H_
