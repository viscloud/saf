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

#ifndef SAF_OPERATOR_DISPLAY_H_
#define SAF_OPERATOR_DISPLAY_H_

#include <memory>
#include <string>

#include "common/types.h"
#include "operator/operator.h"

// Displays frames in an OpenCV window at a specified size ratio and rotation
// angle. After being displayed, frames are passed unchanged to the next
// Operator.
class Display : public Operator {
 public:
  // "key" is the frame key to display, which must correspond to a cv::Mat
  // object. "window_name" is the name to give the display window, which must be
  // unique.
  Display(const std::string& key, unsigned int angle, float size_ratio,
          const std::string& window_name);
  static std::shared_ptr<Display> Create(const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string key_;
  unsigned int angle_;
  float size_ratio_;
  std::string window_name_;
};

#endif  // SAF_OPERATOR_DISPLAY_H_
