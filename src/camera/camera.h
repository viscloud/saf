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

#ifndef SAF_CAMERA_CAMERA_H_
#define SAF_CAMERA_CAMERA_H_

#include "operator/operator.h"
#include "stream/stream.h"

class CameraManager;
/**
 * @brief This class represents a camera available on the device.
 */
class Camera : public Operator {
  friend class CameraManager;

 public:
  Camera(const std::string& name, const std::string& video_uri, int width = -1,
         int height = -1);  // Just a nonsense default value
  virtual std::string GetName() const override;
  std::string GetVideoURI() const;
  std::shared_ptr<Stream> GetStream() const;
  int GetWidth();
  int GetHeight();

  virtual bool Capture(cv::Mat& image);
  virtual CameraType GetCameraType() const = 0;

  // Camera controls
  virtual float GetExposure() = 0;
  virtual void SetExposure(float exposure) = 0;
  virtual float GetSharpness() = 0;
  virtual void SetSharpness(float sharpness) = 0;
  virtual Shape GetImageSize() = 0;
  virtual void SetBrightness(float brightness) = 0;
  virtual float GetBrightness() = 0;
  virtual void SetSaturation(float saturation) = 0;
  virtual float GetSaturation() = 0;
  virtual void SetHue(float hue) = 0;
  virtual float GetHue() = 0;
  virtual void SetGain(float gain) = 0;
  virtual float GetGain() = 0;
  virtual void SetGamma(float gamma) = 0;
  virtual float GetGamma() = 0;
  virtual void SetWBRed(float wb_red) = 0;
  virtual float GetWBRed() = 0;
  virtual void SetWBBlue(float wb_blue) = 0;
  virtual float GetWBBlue() = 0;
  virtual CameraModeType GetMode() = 0;
  virtual void SetImageSizeAndMode(Shape shape, CameraModeType mode) = 0;
  virtual CameraPixelFormatType GetPixelFormat() = 0;
  virtual void SetPixelFormat(CameraPixelFormatType pixel_format) = 0;
  virtual float GetFrameRate() = 0;
  virtual void SetFrameRate(float f) = 0;
  virtual void SetROI(int roi_offset_x, int roi_offset_y, int roi_width,
                      int roi_height) = 0;
  virtual int GetROIOffsetX() = 0;
  virtual int GetROIOffsetY() = 0;
  virtual Shape GetROIOffsetShape() = 0;

  void MoveUp();
  void MoveDown();
  void MoveLeft();
  void MoveRight();

  // Update metadata in frame
  virtual void MetadataToFrame(std::unique_ptr<Frame>& frame);

  std::string GetCameraInfo();
  unsigned long CreateFrameID();

  static const char* kCaptureTimeMicrosKey;

 protected:
  virtual bool Init() override = 0;
  virtual bool OnStop() override = 0;
  virtual void Process() override = 0;

 protected:
  std::string name_;
  std::string video_uri_;
  int width_;
  int height_;
  std::string tile_up_command_;
  std::string tile_down_command_;
  std::string pan_left_command_;
  std::string pan_right_command_;
  // Camera output stream
  std::shared_ptr<Stream> stream_;
  // Counter to set camera specific frame id
  unsigned long frame_id_;
};

#endif  // SAF_CAMERA_CAMERA_H_
