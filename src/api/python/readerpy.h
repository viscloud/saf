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

// Reader wrapper for SAF Python API

#ifndef SAF_API_PYTHON_READERPY_H_
#define SAF_API_PYTHON_READERPY_H_

#include "api/python/framepy.h"
#include "operator/operator.h"
#include "stream/stream.h"

class Readerpy {
 public:
  Readerpy(std::shared_ptr<Operator> op, const std::string output_name);
  std::shared_ptr<Framepy> PopFrame();
  double GetPushFps();
  void Unsubscribe();
  void Stop();

 private:
  StreamReader* streamReader_;
};

#endif  // SAF_API_PYTHON_READERPY_H_
