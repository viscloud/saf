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
#include "camera/camera_manager.h"

TEST(CAMERA_MANAGER_TEST, TEST_BASIC) {
  CameraManager& manager = CameraManager::GetInstance();

  EXPECT_EQ(manager.GetCameras().size(), 3);
  EXPECT_EQ(manager.GetCamera("GST_TEST")->GetWidth(), 640);
}
