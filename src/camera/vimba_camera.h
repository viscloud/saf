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

#ifndef SAF_CAMERA_VIMBA_CAMERA_H_
#define SAF_CAMERA_VIMBA_CAMERA_H_

#include "camera.h"

#include <VimbaCPP/Include/VimbaCPP.h>
#include <VimbaImageTransform/Include/VmbTransform.h>

#define CHECK_VIMBA(cmd)                                                      \
  do {                                                                        \
    VmbErrorType error;                                                       \
    error = (cmd);                                                            \
    if (error != VmbErrorSuccess) {                                           \
      char error_info[256];                                                   \
      VmbGetErrorInfo(error, (VmbANSIChar_t*)error_info, sizeof(error_info)); \
      LOG(FATAL) << "VIMBA Error happened: " << error << " (" << error_info   \
                 << ")";                                                      \
    }                                                                         \
  } while (0)

namespace VmbAPI = AVT::VmbAPI;

class VimbaCameraFrameObserver;

/**
 * @brief A class for AlliedVision camera, the name Vimba comes from
 * AlliedVision's Vimba SDK.
 */
class VimbaCamera : public Camera {
  friend class VimbaCameraFrameObserver;

 public:
  VimbaCamera(const std::string& name, const std::string& video_uri, int width,
              int height, CameraModeType mode = CAMERA_MODE_0,
              CameraPixelFormatType pixel_format = CAMERA_PIXEL_FORMAT_RAW12);
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

  void StopCapture();
  void StartCapture();

 private:
  CameraPixelFormatType VimbaPfmt2CameraPfmt(const std::string& vmb_pfmt);
  std::string CameraPfmt2VimbaPfmt(CameraPixelFormatType pfmt);

  void ResetDefaultCameraSettings();

  CameraPixelFormatType initial_pixel_format_;
  CameraModeType initial_mode_;

  VmbAPI::VimbaSystem& vimba_system_;
  VmbAPI::CameraPtr camera_;
};

#endif  // SAF_CAMERA_VIMBA_CAMERA_H_
