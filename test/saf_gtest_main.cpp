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

#include <stdio.h>

#include <gst/gst.h>
#include <gtest/gtest.h>

#include "common/context.h"
#include "utils/file_utils.h"

GTEST_API_ int main(int argc, char** argv) {
  printf("Running main() from saf_gtest_main.cc\n");
  testing::InitGoogleTest(&argc, argv);
  gst_init(&argc, &argv);
  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  FLAGS_minloglevel = 0;
  if (argc >= 2) {
    std::string config_dir = argv[1];
    LOG(INFO) << config_dir;
    Context::GetContext().SetConfigDir(config_dir);
  } else {
    // Try to guess the test config file
    if (FileExists("./test/config/cameras.toml")) {
      LOG(INFO) << "Use config from ./test/config";
      Context::GetContext().SetConfigDir("./test/config");
    } else if (FileExists("./config/cameras.toml")) {
      LOG(INFO) << "Use config from ./config";
      Context::GetContext().SetConfigDir("./config");
    }
  }

  Context::GetContext().Init();
  return RUN_ALL_TESTS();
}
