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

#ifndef SAF_OPERATOR_FACE_TRACKER_H_
#define SAF_OPERATOR_FACE_TRACKER_H_

#include <cv.h>
#include <boost/optional.hpp>

#include "operator.h"

class FaceTracker : public Operator {
 public:
  FaceTracker(size_t rem_size = 5);
  static std::shared_ptr<FaceTracker> Create(const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  void AttachNearest(std::vector<PointFeature>& point_features,
                     float threshold);
  float GetDistance(const std::vector<float>& a, const std::vector<float>& b);

 private:
  std::list<std::list<boost::optional<PointFeature>>> path_list_;
  size_t rem_size_;
  bool first_frame_;
};

#endif  // SAF_OPERATOR_FACE_TRACKER_H_
