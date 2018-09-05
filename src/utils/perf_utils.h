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

// Utilities related to measuring system performance.

#ifndef SAF_UTILS_PERF_UTILS_H_
#define SAF_UTILS_PERF_UTILS_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>

// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
// Extracts a numeric digit from the line, assuming that a digit exists and the
// line ends in " kB".
inline int ParseMemInfoLine(char* line) {
  int i = strlen(line);
  const char* p = line;
  while (*p < '0' || *p > '9') p++;
  line[i - 3] = '\0';
  i = atoi(p);
  return i;
}

// Returns the value of a memory-related key from "/proc/self/status" (in KB).
inline int GetMemoryInfoKB(std::string key) {
  FILE* file = fopen("/proc/self/status", "r");
  int result = -1;
  char line[128];

  while (fgets(line, 128, file) != NULL) {
    if (strncmp(line, key.c_str(), key.length()) == 0) {
      result = ParseMemInfoLine(line);
      break;
    }
  }
  fclose(file);
  return result;
}

// Returns the physical memory usage (in KB) of the current process.
inline int GetPhysicalKB() { return GetMemoryInfoKB("VmRSS"); }

// Returns the virtual memory usage (in KB) of the current process.
inline int GetVirtualKB() { return GetMemoryInfoKB("VmSize"); }

#endif  //  SAF_UTILS_PERF_UTILS_H_
