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

#ifndef SAF_UTILS_HASH_UTILS_H_
#define SAF_UTILS_HASH_UTILS_H_

#include <cstdint>

inline static uint32_t oat_hash(const char* s) {
  uint32_t h = 0;
  for (; *s; ++s) {
    h += *s;
    h += (h << 10);
    h ^= (h >> 6);
  }

  h += (h << 3);
  h ^= (h >> 11);
  h += (h << 15);

  return h;
}

#endif  // SAF_UTILS_HASH_UTILS_H_
