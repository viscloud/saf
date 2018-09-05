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

// Multi-target tracking using dlib.

#ifndef SAF_OPERATOR_TRACKERS_DLIB_TRACKER_H_
#define SAF_OPERATOR_TRACKERS_DLIB_TRACKER_H_

#include <dlib/dlib/image_processing.h>
#include <dlib/dlib/opencv.h>

#include "operator/trackers/object_tracker.h"

class DlibTracker : public BaseTracker {
 public:
  DlibTracker(const std::string& id, const std::string& tag)
      : BaseTracker(id, tag) {
    impl_.reset(new dlib::correlation_tracker());
  }
  virtual ~DlibTracker() {}
  virtual void Initialize(const cv::Mat& gray_image, cv::Rect bb) {
    dlib::array2d<unsigned char> dlibImageGray;
    dlib::assign_image(dlibImageGray,
                       dlib::cv_image<unsigned char>(gray_image));
    dlib::rectangle initBB(bb.x, bb.y, bb.x + bb.width, bb.y + bb.height);
    impl_->start_track(dlibImageGray, initBB);
  }
  virtual bool IsInitialized() { return true; }
  virtual void Track(const cv::Mat& gray_image) {
    dlib::array2d<unsigned char> dlibImageGray;
    dlib::assign_image(dlibImageGray,
                       dlib::cv_image<unsigned char>(gray_image));
    impl_->update(dlibImageGray);
    auto r = impl_->get_position();
    feat_.resize(124, 0.0);
    bb_ = cv::Rect(r.left(), r.top(), r.right() - r.left(),
                   r.bottom() - r.top()) &
          cv::Rect(0, 0, gray_image.size().width, gray_image.size().height);
    if (bb_.size().width > 0 && bb_.size().height > 0) {
      cv::Mat gray_image_BB = gray_image(bb_);
      cv::resize(gray_image_BB, gray_image_BB, cv::Size(16, 16));
      dlib::array2d<unsigned char> dlibImageGrayBB;
      dlib::assign_image(dlibImageGrayBB,
                         dlib::cv_image<unsigned char>(gray_image_BB));
      dlib::array<dlib::array2d<double>> hogs;
      dlib::extract_fhog_features(dlibImageGrayBB, hogs, 4);
      for (int k = 0, i = 0; i < 31; i++) {
        feat_[k++] = hogs[i][0][0];
        feat_[k++] = hogs[i][0][1];
        feat_[k++] = hogs[i][1][0];
        feat_[k++] = hogs[i][1][1];
      }
    }
  }
  virtual cv::Rect GetBB() { return bb_; }
  virtual std::vector<double> GetBBFeature() { return feat_; }

 private:
  std::unique_ptr<dlib::correlation_tracker> impl_;
  cv::Rect bb_;
  std::vector<double> feat_;
};

#endif  // SAF_OPERATOR_TRACKERS_DLIB_TRACKER_H_
