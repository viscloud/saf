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

#ifndef SAF_CAMERA_CAMERA_MANAGER_H_
#define SAF_CAMERA_CAMERA_MANAGER_H_

#include <string>
#include <unordered_map>
#include "camera/camera.h"
#include "gst_camera.h"

#ifdef USE_PTGRAY
#include "pgr_camera.h"
#endif  // USE_PTGRAY
#ifdef USE_VIMBA
#include "vimba_camera.h"
#endif  // USE_VIMBDA

/**
 * @brief The class that manages and controls all cameras on the device.
 */
class CameraManager {
 public:
  static CameraManager& GetInstance();

 public:
  CameraManager();
  CameraManager(const CameraManager& other) = delete;
  std::unordered_map<std::string, std::shared_ptr<Camera>> GetCameras();
  std::shared_ptr<Camera> GetCamera(const std::string& name);
  bool HasCamera(const std::string& name) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<Camera>> cameras_;
};

#endif  // SAF_CAMERA_CAMERA_MANAGER_H_
