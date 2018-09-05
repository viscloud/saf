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

#include "types.h"

std::string GetCameraPixelFormatString(CameraPixelFormatType pfmt) {
  switch (pfmt) {
    case CAMERA_PIXEL_FORMAT_RAW8:
      return "RAW8";
    case CAMERA_PIXEL_FORMAT_RAW12:
      return "RAW12";
    case CAMERA_PIXEL_FORMAT_MONO8:
      return "Mono8";
    case CAMERA_PIXEL_FORMAT_BGR:
      return "BGR";
    case CAMERA_PIXEL_FORMAT_YUV411:
      return "YUV411";
    case CAMERA_PIXEL_FORMAT_YUV422:
      return "YUV422";
    case CAMERA_PIXEL_FORMAT_YUV444:
      return "YUV444";
    default:
      return "PIXEL_FORMAT_INVALID";
  }
}
