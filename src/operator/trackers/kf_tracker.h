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

// Kalman filter tracker operator

#ifndef SAF_OPERATOR_TRACKERS_KF_TRACKER_H_
#define SAF_OPERATOR_TRACKERS_KF_TRACKER_H_

#include <dlib/filtering.h>
#include <dlib/matrix.h>
#include <dlib/rand.h>
#include <boost/optional.hpp>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

#include "operator/trackers/object_tracker.h"

using dlib::identity_matrix;
using dlib::kalman_filter;
using dlib::matrix;

class KFTracker : public BaseTracker {
 public:
  KFTracker(const std::string& id, const std::string& tag)
      : BaseTracker(id, tag), initialised_(false) {}
  virtual ~KFTracker() {}
  virtual void Initialize(const cv::Mat& gray_image, cv::Rect bb) {
    (void)(gray_image);

    matrix<double, 6, 6> R;

    R = 50, 0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 50, 0,
    0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 50;

    matrix<double, 8, 8> A;

    A = 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0,
    0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1;

    matrix<double, 6, 8> H;

    H = 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1;

    kf.set_measurement_noise(R);
    matrix<double> pn = 0.01 * identity_matrix<double, 8>();
    kf.set_process_noise(pn);
    kf.set_observation_model(H);
    kf.set_transition_model(A);

    matrix<double, 8, 1> x;

    bb_ = bb;
    feat_.resize(124, 0.0);
    cv::Point cpt = GetCenterPoint(bb_);
    x = cpt.x, cpt.y, 0, 0, bb_.x, bb_.y, (bb_.x + bb_.width),
    (bb_.y + bb_.height);  // This is the initial bbox center

    kf.set_state(x);

    last_calibration_time_ = std::chrono::system_clock::now();
    initialised_ = true;
  }
  virtual bool IsInitialized() { return initialised_; }
  virtual void Track(const cv::Mat& gray_image) {
    (void)(gray_image);

    kf.update();  // on every frame

    xb = kf.get_predicted_next_state();

    cv::Point predict_pt = cv::Point(xb(0, 0), xb(1, 0));
    bb_ = GetRect(predict_pt, xb(6, 0) - xb(4, 0), xb(7, 0) - xb(5, 0));
  }
  virtual cv::Rect GetBB() { return bb_; }
  virtual std::vector<double> GetBBFeature() { return feat_; }
  virtual bool TrackGetPossibleBB(const cv::Mat& gray_image,
                                  std::vector<Rect>& untracked_bboxes,
                                  std::vector<std::string>& untracked_tags,
                                  cv::Rect& rt) {
    (void)(gray_image);

    rt = GetBB();

    boost::optional<cv::Rect> best_rect;
    double min_dist = std::numeric_limits<double>::max();
    size_t min_i;
    for (size_t i = 0; i < untracked_bboxes.size(); ++i) {
      cv::Rect ru(untracked_bboxes[i].px, untracked_bboxes[i].py,
                  untracked_bboxes[i].width, untracked_bboxes[i].height);
      auto pu = GetCenterPoint(ru);
      auto pt = GetCenterPoint(rt);
      auto dist = GetDistance(pu, pt);
      if (dist < min_dist) {
        min_dist = dist;
        min_i = i;
      }
    }

    if (min_dist < 50) {
      cv::Rect r_tmp(untracked_bboxes[min_i].px, untracked_bboxes[min_i].py,
                     untracked_bboxes[min_i].width,
                     untracked_bboxes[min_i].height);
      best_rect = r_tmp;
      untracked_bboxes.erase(untracked_bboxes.begin() + min_i);
      untracked_tags.erase(untracked_tags.begin() + min_i);
    }

    auto now = std::chrono::system_clock::now();
    if (best_rect) {
      cv::Point cpt = GetCenterPoint(*best_rect);
      matrix<double, 6, 1> z;
      z = cpt.x, cpt.y, best_rect->x, best_rect->y,
      (best_rect->x + best_rect->width), (best_rect->y + best_rect->height);
      kf.update(z);
      matrix<double, 8, 1> x;
      x = kf.get_current_state();
      cv::Point curr_pt(x(0, 0), x(1, 0));
      // use current width height
      rt = GetRect(curr_pt, x(6, 0) - x(4, 0), x(7, 0) - x(5, 0));
      bb_ = rt;
      last_calibration_time_ = now;
    } else {
      kf.update();
      xb = kf.get_predicted_next_state();
      cv::Point predict_pt = cv::Point(xb(0, 0), xb(1, 0));
      rt = GetRect(predict_pt, xb(6, 0) - xb(4, 0), xb(7, 0) - xb(5, 0));
      bb_ = rt;
    }

    std::chrono::duration<double> diff = now - last_calibration_time_;
    return diff.count() < 1;
  }

 private:
  cv::Point GetCenterPoint(const cv::Rect& rect) {
    cv::Point cpt;
    cpt.x = rect.x + cvRound(rect.width / 2.0);
    cpt.y = rect.y + cvRound(rect.height / 2.0);
    return cpt;
  }
  cv::Rect GetRect(const cv::Point& pt, int width, int height) {
    int xmin = pt.x - cvRound(width / 2.0);
    int ymin = pt.y - cvRound(height / 2.0);
    return cv::Rect(xmin, ymin, width, height);
  }
  double GetDistance(cv::Point pointO, cv::Point pointA) {
    double distance;
    distance =
        std::pow((pointO.x - pointA.x), 2) + std::pow((pointO.y - pointA.y), 2);
    distance = std::sqrt(distance);
    return distance;
  }
  kalman_filter<8, 6> kf;
  bool initialised_;
  matrix<double, 8, 1> xb;
  cv::Rect bb_;
  std::vector<double> feat_;
  std::chrono::time_point<std::chrono::system_clock> last_calibration_time_;
};

#endif  // SAF_OPERATOR_TRACKERS_KF_TRACKER_H_
