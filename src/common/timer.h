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

#ifndef SAF_COMMON_TIMER_H_
#define SAF_COMMON_TIMER_H_

#include <chrono>

/**
 * @brief Class used to measure wall clock time.
 */
class Timer {
 public:
  typedef std::chrono::time_point<std::chrono::system_clock> TimerTimePoint;

 public:
  Timer() {}

  /**
   * @brief Start the timer, this will clear the start time.
   */
  void Start() { start_time_ = std::chrono::system_clock::now(); }

  /**
   * @brief Get elapsed time in milliseconds.
   */
  double ElapsedMSec() { return ElapsedMicroSec() / 1000.0; }

  /**
   * @brief Get elapsed time in seconds.
   */
  double ElapsedSec() { return ElapsedMicroSec() / 1000000.0; }

  /**
   * @brief Get elapsed time in micro seconds.
   */
  double ElapsedMicroSec() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now() - start_time_)
        .count();
  }

 private:
  TimerTimePoint start_time_;
};

#endif  // SAF_COMMON_TIMER_H_
