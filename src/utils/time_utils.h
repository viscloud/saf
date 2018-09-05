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

#ifndef SAF_UTILS_TIME_UTILS_H_
#define SAF_UTILS_TIME_UTILS_H_

#include <chrono>
#include <sstream>
#include <string>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

// Returns a string encoding of the provided ptime.
inline std::string GetDateTimeString(boost::posix_time::ptime time) {
  return boost::posix_time::to_iso_extended_string(time);
}

// Returns a string encoding of the current time.
inline std::string GetCurrentDateTimeString() {
  return GetDateTimeString(boost::posix_time::microsec_clock::local_time());
}

// Returns a string specifying the full directory path, starting with
// "base_dir", that ends with a hierarchy based on the provided ptime. The
// hierarchy includes levels for the day and hour.
inline std::string GetDateTimeDir(std::string base_dir,
                                  boost::posix_time::ptime time) {
  std::string date = boost::gregorian::to_iso_extended_string(time.date());
  long hours = time.time_of_day().hours();

  std::ostringstream dirpath;
  dirpath << base_dir << "/" << date << "/" << hours << "/";
  return dirpath.str();
}

inline unsigned long GetTimeSinceEpochMillis() {
  return static_cast<unsigned long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

inline unsigned long GetTimeSinceEpochMicros(boost::posix_time::ptime time) {
  boost::posix_time::ptime epoch =
      boost::posix_time::time_from_string("1970-01-01 00:00:00.000");
  return static_cast<unsigned long>((time - epoch).total_microseconds());
}

#endif  // SAF_UTILS_TIME_UTILS_H_
