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

#ifndef SAF_UTILS_OUTPUT_TRACKER_H_
#define SAF_UTILS_OUTPUT_TRACKER_H_

#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

class OutputTracker {
 public:
  OutputTracker(const std::string& root_dir, bool organize_by_time,
                unsigned long frames_per_dir);
  std::string GetAndCreateOutputDir(boost::posix_time::ptime micros);
  std::string GetRootDir();

 private:
  void ChangeSubdir(unsigned long subdir_idx);

  std::string root_dir_;
  bool organize_by_time_;

  unsigned long frames_per_dir_;
  unsigned long frames_in_current_dir_;
  unsigned long current_dir_idx_;
  std::string current_dirpath_;
};

#endif  // SAF_UTILS_OUTPUT_TRACKER_H_
