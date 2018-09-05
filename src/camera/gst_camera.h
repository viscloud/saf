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

#ifndef SAF_CAMERA_GST_CAMERA_H_
#define SAF_CAMERA_GST_CAMERA_H_

#include "camera/camera.h"
#include "video/gst_video_capture.h"

class GSTCamera : public Camera {
 public:
  GSTCamera(const std::string& name, const std::string& video_uri,
            int width = -1, int height = -1, unsigned long max_buf_size = 10,
            bool restart = true);
  // Must be set before calling Init()
  // Otherwise it will default to no file output
  void SetOutputFilepath(const std::string& output_filepath);
  void SetFileFramerate(unsigned int file_framerate);
  virtual CameraType GetCameraType() const override;

  virtual float GetExposure() override;
  virtual void SetExposure(float exposure) override;
  virtual float GetSharpness() override;
  virtual void SetSharpness(float sharpness) override;
  virtual Shape GetImageSize() override;
  virtual void SetBrightness(float brightness) override;
  virtual float GetBrightness() override;
  virtual void SetSaturation(float saturation) override;
  virtual float GetSaturation() override;
  virtual void SetHue(float hue) override;
  virtual float GetHue() override;
  virtual void SetGain(float gain) override;
  virtual float GetGain() override;
  virtual void SetGamma(float gamma) override;
  virtual float GetGamma() override;
  virtual void SetWBRed(float wb_red) override;
  virtual float GetWBRed() override;
  virtual void SetWBBlue(float wb_blue) override;
  virtual float GetWBBlue() override;
  virtual CameraModeType GetMode() override;
  virtual void SetImageSizeAndMode(Shape shape, CameraModeType mode) override;
  virtual CameraPixelFormatType GetPixelFormat() override;
  virtual void SetPixelFormat(CameraPixelFormatType pixel_format) override;
  virtual void SetFrameRate(float f) override;
  virtual float GetFrameRate() override;
  virtual void SetROI(int roi_offset_x, int roi_offset_y, int roi_width,
                      int roi_height) override;
  virtual int GetROIOffsetX() override;
  virtual int GetROIOffsetY() override;
  virtual Shape GetROIOffsetShape() override;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  GstVideoCapture capture_;
  std::string output_filepath_;
  unsigned int file_framerate_;
};

#endif  // SAF_CAMERA_GST_CAMERA_H_
