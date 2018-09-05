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

#ifndef SAF_OPERATOR_FRAME_WRITER_H_
#define SAF_OPERATOR_FRAME_WRITER_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "common/types.h"
#include "operator/operator.h"
#include "utils/output_tracker.h"

// The FrameWriter writes Frames to disk in either binary, JSON, or text format.
// The user can specify which frame fields to save (the default is all fields).
class FrameWriter : public Operator {
 public:
  enum FileFormat { BINARY, JSON, TEXT };

  // "fields" is a set of frame fields to save. If "fields" is an empty set,
  // then all fields will be saved. If "save_fields_separately" is true, then
  // field will be saved in a separate file. If "organize_by_time" is true, then
  // the frames will be stored in a date-time directory hierarchy and
  // "frames_per_dir" will be ignored. If "organize_by_time" is false, then
  // "frames_per_dir" will determine the number of frames saved in each output
  // subdirectory.
  FrameWriter(const std::unordered_set<std::string> fields = {},
              const std::string& output_dir = ".",
              const FileFormat format = TEXT,
              bool save_fields_separately = false,
              bool organize_by_time = false,
              unsigned long frames_per_dir = 1000);

  // "params" should contain three keys, "fields" (which should be a set of
  // field names), "output_dir", and "format" (which should be the textual
  // representation of the FileFormat value).
  static std::shared_ptr<FrameWriter> Create(const FactoryParamsType& params);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string GetExtension();

  // The frame fields to save.
  std::unordered_set<std::string> fields_;
  // The file format in which to save frames.
  FileFormat format_;
  // Whether to save each field in a separate file.
  bool save_fields_separately_;
  // Tracks which directory frames should be written to.
  OutputTracker tracker_;
};

#endif  // SAF_OPERATOR_FRAME_WRITER_H_
