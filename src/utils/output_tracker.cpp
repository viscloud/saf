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

#include "utils/output_tracker.h"

#include <sstream>
#include <stdexcept>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include "utils/file_utils.h"

OutputTracker::OutputTracker(const std::string& root_dir, bool organize_by_time,
                             unsigned long frames_per_dir)
    : root_dir_(root_dir),
      organize_by_time_(organize_by_time),
      frames_per_dir_(frames_per_dir),
      frames_in_current_dir_(0),
      current_dir_idx_(0),
      current_dirpath_("") {
  if (!boost::filesystem::exists(root_dir_)) {
    std::ostringstream msg;
    msg << "Desired output directory \"" << root_dir_ << "\" does not exist!";
    throw std::runtime_error(msg.str());
  }

  if (!organize_by_time_) {
    ChangeSubdir(0);
  }
}

std::string OutputTracker::GetAndCreateOutputDir(
    boost::posix_time::ptime micros) {
  if (organize_by_time_) {
    return GetAndCreateDateTimeDir(root_dir_, micros);
  } else {
    std::string dir = current_dirpath_;
    ++frames_in_current_dir_;
    if (frames_in_current_dir_ == frames_per_dir_) {
      // If we have filled up the current subdir, then move on to the next one.
      ChangeSubdir(current_dir_idx_ + 1);
    }
    return dir;
  }
}

std::string OutputTracker::GetRootDir() { return root_dir_; }

void OutputTracker::ChangeSubdir(unsigned long subdir_idx) {
  frames_in_current_dir_ = 0;
  current_dir_idx_ = subdir_idx;

  std::ostringstream current_dirpath;
  current_dirpath << root_dir_ << "/" << current_dir_idx_ << "/";
  current_dirpath_ = current_dirpath.str();
  CreateDirs(current_dirpath_);
}
