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

#include "framepy.h"

Framepy::Framepy(std::shared_ptr<Frame> f) : frame_(f) {}

PyObject* Framepy::GetValue(const std::string key) const {
  if (key.compare("original_image") == 0) {
    cv::Mat m = frame_->GetValue<cv::Mat>(key);
    return pbcvt::fromMatToNDArray(m);
  }
  // TODO: Handle other types
  Py_RETURN_NONE;
}
