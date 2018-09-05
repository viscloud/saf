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
#include <tinytoml/include/toml/toml.h>

TEST(TINYTOML_TEST, TINYTOML_TEST) {
  std::string test_toml_string =
      "arr1 = [ 1, 2, 3 ]\n"
      "arr2 = [ \"red\", \"yellow\", \"green\" ]\n"
      "arr3 = [ [ 1, 2 ], [3, 4, 5] ]\n"
      "arr4 = [ \"all\", 'strings', \"\"\"are the same\"\"\", '''type'''] # "
      "this is ok\n"
      "arr5 = [ [ 1, 2 ], [\"a\", \"b\", \"c\"] ] # this is ok\n"
      "# arr6 = [ 1, 2.0 ] # note: this is NOT ok\n"
      "arr7 = [\n"
      "  1, 2, 3\n"
      "]\n"
      "\n"
      "arr8 = [\n"
      "  1,\n"
      "  2, # this is ok\n"
      "]";
  std::stringstream ss(test_toml_string);
  toml::ParseResult pr = toml::parse(ss);

  EXPECT_TRUE(pr.valid());
  const toml::Value& v = pr.value;

  const toml::Value* x = v.find("arr1");
  EXPECT_TRUE(x != nullptr);

  std::vector<int> arr1 = v.get<std::vector<int>>("arr1");
  EXPECT_EQ(arr1[0], 1);
  EXPECT_EQ(arr1[1], 2);
  EXPECT_EQ(arr1[2], 3);

  EXPECT_TRUE(v.find("arr6") == nullptr);
}
