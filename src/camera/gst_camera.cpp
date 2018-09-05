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

#include "camera/gst_camera.h"

#include <chrono>

#include "utils/string_utils.h"

GSTCamera::GSTCamera(const std::string& name, const std::string& video_uri,
                     int width, int height, unsigned long max_buf_size,
                     bool restart)
    : Camera(name, video_uri, width, height),
      capture_{max_buf_size, restart},
      output_filepath_(""),
      file_framerate_(0) {}

bool GSTCamera::Init() {
  bool opened =
      capture_.CreatePipeline(video_uri_, output_filepath_, file_framerate_);

  // Determine if we should block when pushing frames.
  std::string video_protocol;
  std::string video_path;
  ParseProtocolAndPath(video_uri_, video_protocol, video_path);
  if (video_protocol == "file") {
    // If we're reading from a file, then by default we don't want the camera to
    // drop any frames.
    SetBlockOnPush(true);
  }

  if (!opened) {
    LOG(INFO) << "can't open camera";
    return false;
  }

  return true;
}

void GSTCamera::SetOutputFilepath(const std::string& output_filepath) {
  output_filepath_ = output_filepath;
}
void GSTCamera::SetFileFramerate(unsigned int file_framerate) {
  file_framerate_ = file_framerate;
}

bool GSTCamera::OnStop() {
  capture_.DestroyPipeline();
  return true;
}
void GSTCamera::Process() {
  auto frame = std::make_unique<Frame>();
  MetadataToFrame(frame);

  if (capture_.NextFrameIsLast()) {
    frame->SetStopFrame(true);
  } else {
    const cv::Mat& pixels =
        capture_.GetPixels(frame->GetValue<unsigned long>("frame_id"));
    if (pixels.empty()) {
      // Did not get a new frame.
      return;
    }

    frame->SetValue("original_bytes",
                    std::vector<char>((char*)pixels.data,
                                      (char*)pixels.data +
                                          pixels.total() * pixels.elemSize()));
    frame->SetValue("original_image", pixels);
  }
  PushFrame("output", std::move(frame));
}

CameraType GSTCamera::GetCameraType() const { return CAMERA_TYPE_GST; }

// TODO: Implement camera control for GST camera, we may need to have subclass
// for IP camera, and use GstVideoCapture there.
float GSTCamera::GetExposure() { return 0; }
void GSTCamera::SetExposure(float) {}
float GSTCamera::GetSharpness() { return 0; }
void GSTCamera::SetSharpness(float) {}
Shape GSTCamera::GetImageSize() { return Shape(); }
void GSTCamera::SetBrightness(float) {}
float GSTCamera::GetBrightness() { return 0; }
void GSTCamera::SetSaturation(float) {}
float GSTCamera::GetSaturation() { return 0; }
void GSTCamera::SetHue(float) {}
float GSTCamera::GetHue() { return 0; }
void GSTCamera::SetGain(float) {}
float GSTCamera::GetGain() { return 0; }
void GSTCamera::SetGamma(float) {}
float GSTCamera::GetGamma() { return 0; }
void GSTCamera::SetWBRed(float) {}
float GSTCamera::GetWBRed() { return 0; }
void GSTCamera::SetWBBlue(float) {}
float GSTCamera::GetWBBlue() { return 0; }
CameraModeType GSTCamera::GetMode() { return CAMERA_MODE_INVALID; }
void GSTCamera::SetImageSizeAndMode(Shape, CameraModeType) {}
CameraPixelFormatType GSTCamera::GetPixelFormat() {
  return CAMERA_PIXEL_FORMAT_INVALID;
}
void GSTCamera::SetPixelFormat(CameraPixelFormatType) {}
void GSTCamera::SetFrameRate(float) {}
float GSTCamera::GetFrameRate() { return 0; }
void GSTCamera::SetROI(int, int, int, int) {}
int GSTCamera::GetROIOffsetX() { return 0; }
int GSTCamera::GetROIOffsetY() { return 0; }
Shape GSTCamera::GetROIOffsetShape() { return Shape(); }
