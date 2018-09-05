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

#ifndef SAF_CAMERA_PGR_CAMERA_H_
#define SAF_CAMERA_PGR_CAMERA_H_

#include <flycapture/FlyCapture2.h>

#include "camera.h"
#include "utils/utils.h"

/**
 * @brief A class for ptgray camera, in order to use this class, you have to
 * make sure that the SDK for ptgray camera is installed on the system.
 */
class PGRCamera : public Camera {
 public:
  PGRCamera(const std::string& name, const std::string& video_uri,
            int width = -1, int height = -1,
            CameraModeType mode = CAMERA_MODE_0,
            CameraPixelFormatType pixel_format = CAMERA_PIXEL_FORMAT_RAW12);
  virtual CameraType GetCameraType() const override;

  // Camera controls
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
  void Reset();

 private:
  /**
   * @brief Set the property of the camera in either int value or abs value.
   * @param property_type The type of the property.
   * @param value The value of the property.
   * @param abs Wether the value should be set as abs value (float) or not.
   * @param int_value_idx Set value_a or value_b.
   */
  void SetProperty(FlyCapture2::PropertyType property_type, float value,
                   bool abs, bool value_a = true);
  float GetProperty(FlyCapture2::PropertyType property_type, bool abs,
                    bool value_a = true);
  FlyCapture2::Format7ImageSettings GetImageSettings();

  static void OnImageGrabbed(FlyCapture2::Image* image, const void* user_data);

  CameraModeType FCMode2CameraMode(FlyCapture2::Mode fc_mode);
  FlyCapture2::Mode CameraMode2FCMode(CameraModeType mode);
  CameraPixelFormatType FCPfmt2CameraPfmt(FlyCapture2::PixelFormat fc_pfmt);
  FlyCapture2::PixelFormat CameraPfmt2FCPfmt(CameraPixelFormatType pfmt);

  CameraPixelFormatType initial_pixel_format_;
  CameraModeType initial_mode_;

  FlyCapture2::Camera camera_;
  std::mutex camera_lock_;
};

#endif  // SAF_CAMERA_PGR_CAMERA_H_
