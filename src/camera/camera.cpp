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

#include "camera/camera.h"

#include <stdexcept>

#include "common/types.h"
#include "utils/time_utils.h"

const char* Camera::kCaptureTimeMicrosKey = "capture_time_micros";

Camera::Camera(const std::string& name, const std::string& video_uri, int width,
               int height)
    : Operator(OPERATOR_TYPE_CAMERA, {}, {"output"}),
      name_(name),
      video_uri_(video_uri),
      width_(width),
      height_(height),
      frame_id_(0) {
  stream_ = sinks_["output"];
}

std::string Camera::GetName() const { return name_; }

std::string Camera::GetVideoURI() const { return video_uri_; }

int Camera::GetWidth() { return width_; }
int Camera::GetHeight() { return height_; }

std::shared_ptr<Stream> Camera::GetStream() const { return stream_; }

unsigned long Camera::CreateFrameID() { return frame_id_++; }

bool Camera::Capture(cv::Mat& image) {
  if (stopped_) {
    LOG(WARNING) << "stopped.";
    Start();
    auto reader = stream_->Subscribe();
    // Pop the first 3 images, the first few shots of the camera might be
    // garbage
    for (int i = 0; i < 3; i++) {
      reader->PopFrame();
    }
    auto frame = reader->PopFrame();
    if (frame == nullptr) {
      throw std::runtime_error("Got null frame");
    }
    image = frame->GetValue<cv::Mat>("original_image");
    reader->UnSubscribe();
    Stop();
  } else {
    LOG(WARNING) << "not stopped.";
    auto reader = stream_->Subscribe();
    auto frame = reader->PopFrame();
    if (frame == nullptr) {
      throw std::runtime_error("Got null frame");
    }
    image = frame->GetValue<cv::Mat>("original_image");
    reader->UnSubscribe();
  }

  return true;
}

/**
 * Get the camera parameters information.
 */
std::string Camera::GetCameraInfo() {
  std::ostringstream ss;
  ss << "name: " << GetName() << "\n";
  ss << "record time: " << GetCurrentDateTimeString() << "\n";
  ss << "image size: " << GetImageSize().width << "x" << GetImageSize().height
     << "\n";
  ss << "pixel format: " << GetCameraPixelFormatString(GetPixelFormat())
     << "\n";
  ss << "exposure: " << GetExposure() << "\n";
  ss << "gain: " << GetGain() << "\n";
  return ss.str();
}

void Camera::MetadataToFrame(std::unique_ptr<Frame>& frame) {
  frame->SetValue("camera_name", GetName());
  frame->SetValue(Frame::kFrameIdKey, CreateFrameID());
  frame->SetValue(kCaptureTimeMicrosKey,
                  boost::posix_time::microsec_clock::local_time());
  frame->SetValue("CameraSettings.Exposure", GetExposure());
  frame->SetValue("CameraSettings.Sharpness", GetSharpness());
  frame->SetValue("CameraSettings.Brightness", GetBrightness());
  frame->SetValue("CameraSettings.Saturation", GetSaturation());
  frame->SetValue("CameraSettings.Hue", GetHue());
  frame->SetValue("CameraSettings.Gain", GetGain());
  frame->SetValue("CameraSettings.Gamma", GetGamma());
  frame->SetValue("CameraSettings.WBRed", GetWBRed());
  frame->SetValue("CameraSettings.WBBlue", GetWBBlue());
}

/*****************************************************************************
 * Pan/Tile, the implementation is ugly, but it is ok to show that we can
 * pan/tile the camera programmably.
 * TODO: A more general, and extendable way to unify this.
 *****************************************************************************/
void Camera::MoveUp() { ExecuteAndCheck(tile_up_command_ + " &"); }
void Camera::MoveDown() { ExecuteAndCheck((tile_down_command_ + " &")); }
void Camera::MoveLeft() { ExecuteAndCheck((pan_left_command_ + " &")); }
void Camera::MoveRight() { ExecuteAndCheck((pan_right_command_ + " &")); }

void Camera::PushFrame(const std::string& sink_name,
                       std::unique_ptr<Frame> frame) {
  auto img = frame->GetValue<cv::Mat>("original_image");
  int actual_width = img.cols;
  int actual_height = img.rows;
  int expected_width = GetWidth();
  int expected_height = GetHeight();
  if ((actual_width != expected_width) || (actual_height != expected_height)) {
    LOG(ERROR) << "Actual dimensions of frame "
               << frame->GetValue<unsigned long>(Frame::kFrameIdKey) << " ("
               << actual_width << " x " << actual_height
               << ") do not match expected frame dimensions (" << expected_width
               << " x " << expected_height << ")!";
  }
  Operator::PushFrame(sink_name, std::move(frame));
}
