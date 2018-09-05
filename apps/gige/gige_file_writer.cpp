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

#include "gige_file_writer.h"

#include <boost/filesystem.hpp>

GigeFileWriter::GigeFileWriter(const std::string& directory,
                               size_t frames_per_file)
    : Operator(OPERATOR_TYPE_CUSTOM, {"input"}, {}),
      directory_(directory),
      frames_written_(0),
      frames_per_file_(frames_per_file) {}

bool GigeFileWriter::Init() {
  // Make a directory for this run
  if (!boost::filesystem::exists(directory_)) {
    boost::filesystem::create_directory(directory_);
  } else {
    LOG(WARNING) << "Directory: " << directory_
                 << " already exists, may re-write existed files";
  }

  frames_written_ = 0;
  current_filename_ = "";

  return true;
}

bool GigeFileWriter::OnStop() {
  if (current_file_.is_open()) {
    current_file_.close();
  }
  return true;
}

void GigeFileWriter::Process() {
  // Create file
  if (frames_written_ % frames_per_file_ == 0) {
    std::ostringstream ss;
    ss << frames_written_ / frames_per_file_ << ".dat";

    std::string filename = directory_ + "/" + ss.str();

    if (current_file_.is_open()) current_file_.close();

    current_file_.open(filename, std::ios::binary | std::ios::out);

    if (!current_file_.is_open()) {
      LOG(FATAL) << "Can't open file: " << filename << " for write";
    }

    current_filename_ = filename;
  }

  auto frame = GetFrame("input");

  std::vector<char> raw_pixels =
      frame->GetValue<std::vector<char>>("original_bytes");
  current_file_.write((char*)raw_pixels.data(), raw_pixels.size());

  frames_written_ += 1;
}
