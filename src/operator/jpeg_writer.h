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

#ifndef SAF_OPERATOR_JPEG_WRITER_H_
#define SAF_OPERATOR_JPEG_WRITER_H_

#include <memory>
#include <string>

#include "common/types.h"
#include "operator/operator.h"
#include "utils/output_tracker.h"

// This Operator encodes a specified field from each frame as a JPEG file using
// the default JPEG compression settings. Therefore, the specified field must
// represent an image stored as a cv::Mat. The resulting files are named using
// the frame's "capture_time_micros" field and the name of the field that is
// being written. The path to the resulting JPEG file is stored in the frame.
class JpegWriter : public Operator {
 public:
  // "field" denotes which field to encode, "output_dir" denotes the directory
  // in which the resulting files will be written, and "num_frames_per_dir" is
  // number of frames to put in each subdir in "output_dir".
  JpegWriter(const std::string& field = "original_image",
             const std::string& output_dir = ".", bool organize_by_time = false,
             unsigned long frames_per_dir = 1000);

  // "params" should contain two fields, "field" and "output_dir"
  static std::shared_ptr<JpegWriter> Create(const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

  static const char* kPathKey;
  static const char* kFieldKey;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  // The frame field that will be encoded.
  std::string field_;
  // Tracks which directory frames should be written to.
  OutputTracker tracker_;
};

#endif  // SAF_OPERATOR_JPEG_WRITER_H_
