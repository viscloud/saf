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

#include <gtest/gtest.h>
#include "utils/string_utils.h"

TEST(STRING_UTILS_TEST, GET_IP_ADDR_FROM_STRING_TEST) {
  std::string ip_addr = "1.2.3.4";
  unsigned ip_val = GetIPAddrFromString(ip_addr);
  EXPECT_EQ(ip_val, 0x01020304);
}
