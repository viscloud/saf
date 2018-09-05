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

#include "camera/pgr_camera.h"

#include "utils/string_utils.h"
#include "utils/utils.h"

#define CHECK_PGR(cmd)                       \
  do {                                       \
    FlyCapture2::Error error;                \
    error = (cmd);                           \
    if (error != FlyCapture2::PGRERROR_OK) { \
      error.PrintErrorTrace();               \
      LOG(FATAL) << "PGR Error happend";     \
    }                                        \
  } while (0)

PGRCamera::PGRCamera(const std::string& name, const std::string& video_uri,
                     int width, int height, CameraModeType mode,
                     CameraPixelFormatType pixel_format)
    : Camera(name, video_uri, width, height),
      initial_pixel_format_(pixel_format),
      initial_mode_(mode) {}

bool PGRCamera::Init() {
  // Get the camera guid from ip address
  std::string protocol, ip;
  ParseProtocolAndPath(video_uri_, protocol, ip);

  FlyCapture2::BusManager bus_manager;
  // Might be a valid ip address
  FlyCapture2::PGRGuid guid;  // GUID of the camera.
  if (StringContains(ip, ".")) {
    unsigned int ip_addr = GetIPAddrFromString(ip);

    CHECK_PGR(bus_manager.GetCameraFromIPAddress(ip_addr, &guid));
  } else {
    // Not ip address, might be the device index
    unsigned int device_idx = (unsigned int)StringToInt(ip);
    unsigned int num_cameras;
    CHECK_PGR(bus_manager.GetNumOfCameras(&num_cameras));
    CHECK(device_idx < num_cameras) << "Invalid camera index: " << device_idx;
    CHECK_PGR(bus_manager.GetCameraFromIndex(device_idx, &guid));
  }

  CHECK_PGR(camera_.Connect(&guid));

  FlyCapture2::Format7ImageSettings fmt7ImageSettings;
  fmt7ImageSettings.mode = CameraMode2FCMode(initial_mode_);
  fmt7ImageSettings.offsetX = 0;
  fmt7ImageSettings.offsetY = 0;
  fmt7ImageSettings.width = (unsigned)width_;
  fmt7ImageSettings.height = (unsigned)height_;
  fmt7ImageSettings.pixelFormat = CameraPfmt2FCPfmt(initial_pixel_format_);

  bool valid;
  FlyCapture2::Format7PacketInfo fmt7PacketInfo;
  CHECK_PGR(camera_.ValidateFormat7Settings(&fmt7ImageSettings, &valid,
                                            &fmt7PacketInfo));
  CHECK_PGR(camera_.SetFormat7Configuration(
      &fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket));

  camera_.StartCapture(PGRCamera::OnImageGrabbed, this);
  Reset();

  LOG(INFO) << "Camera initialized";

  return true;
}

void PGRCamera::OnImageGrabbed(FlyCapture2::Image* raw_image,
                               const void* user_data) {
  PGRCamera* camera = (PGRCamera*)user_data;

  FlyCapture2::Image converted_image;
  std::vector<char> image_bytes(
      (char*)raw_image->GetData(),
      (char*)raw_image->GetData() +
          raw_image->GetDataSize() * sizeof(raw_image->GetData()[0]));
  raw_image->Convert(FlyCapture2::PIXEL_FORMAT_BGR, &converted_image);

  unsigned int rowBytes =
      static_cast<unsigned>((double)converted_image.GetReceivedDataSize() /
                            (double)converted_image.GetRows());

  cv::Mat image = cv::Mat(converted_image.GetRows(), converted_image.GetCols(),
                          CV_8UC3, converted_image.GetData(), rowBytes);

  cv::Mat output_image = image.clone();
  auto frame = std::make_unique<Frame>();
  camera->MetadataToFrame(frame);
  frame->SetValue("original_bytes", image_bytes);
  frame->SetValue("original_image", output_image);
  camera->PushFrame("output", std::move(frame));
}

bool PGRCamera::OnStop() {
  camera_.StopCapture();
  camera_.Disconnect();
  return true;
}

void PGRCamera::Process() {
  // Do nothing here, frames are handled in the callback OnImageGrabbed()
}

float PGRCamera::GetExposure() {
  return GetProperty(FlyCapture2::AUTO_EXPOSURE, true, false);
}
void PGRCamera::SetExposure(float exposure) {
  SetProperty(FlyCapture2::AUTO_EXPOSURE, exposure, true, false);
}
float PGRCamera::GetSharpness() {
  return GetProperty(FlyCapture2::SHARPNESS, false, true);
}
void PGRCamera::SetSharpness(float sharpness) {
  SetProperty(FlyCapture2::SHARPNESS, sharpness, false, true);
}

void PGRCamera::SetBrightness(float brightness) {
  brightness = std::max(0.0f, brightness);
  SetProperty(FlyCapture2::BRIGHTNESS, brightness, true, false);
}

float PGRCamera::GetBrightness() {
  return GetProperty(FlyCapture2::BRIGHTNESS, true, false);
}

void PGRCamera::SetSaturation(float saturation) {
  SetProperty(FlyCapture2::SATURATION, saturation, true, false);
}

float PGRCamera::GetSaturation() {
  return GetProperty(FlyCapture2::SATURATION, true, false);
}

void PGRCamera::SetHue(float hue) {
  SetProperty(FlyCapture2::HUE, hue, true, false);
}
float PGRCamera::GetHue() { return GetProperty(FlyCapture2::HUE, true, false); }

void PGRCamera::SetGain(float gain) {
  SetProperty(FlyCapture2::GAIN, gain, true, false);
}

float PGRCamera::GetGain() {
  return GetProperty(FlyCapture2::GAIN, true, false);
}
void PGRCamera::SetGamma(float gamma) {
  SetProperty(FlyCapture2::GAMMA, gamma, true, false);
}
float PGRCamera::GetGamma() {
  return GetProperty(FlyCapture2::GAMMA, true, false);
}
void PGRCamera::SetWBRed(float wb_red) {
  FlyCapture2::Property prop;
  prop.type = FlyCapture2::WHITE_BALANCE;
  prop.onOff = true;
  prop.autoManualMode = false;
  prop.absControl = false;
  prop.valueA = (unsigned int)wb_red;
  prop.valueB = (unsigned int)GetWBBlue();

  CHECK_PGR(camera_.SetProperty(&prop));
}
float PGRCamera::GetWBRed() {
  return GetProperty(FlyCapture2::WHITE_BALANCE, false, true);
}
void PGRCamera::SetWBBlue(float wb_blue) {
  FlyCapture2::Property prop;
  prop.type = FlyCapture2::WHITE_BALANCE;
  prop.onOff = true;
  prop.autoManualMode = false;
  prop.absControl = false;
  prop.valueA = (unsigned int)GetWBRed();
  prop.valueB = (unsigned int)wb_blue;

  CHECK_PGR(camera_.SetProperty(&prop));
}

float PGRCamera::GetWBBlue() {
  return GetProperty(FlyCapture2::WHITE_BALANCE, false, false);
}

FlyCapture2::Format7ImageSettings PGRCamera::GetImageSettings() {
  FlyCapture2::Format7ImageSettings image_settings;
  unsigned int current_packet_size;
  float current_percentage;
  CHECK_PGR(camera_.GetFormat7Configuration(
      &image_settings, &current_packet_size, &current_percentage));
  return image_settings;
}

CameraPixelFormatType PGRCamera::GetPixelFormat() {
  auto image_settings = GetImageSettings();
  return FCPfmt2CameraPfmt(image_settings.pixelFormat);
}

Shape PGRCamera::GetImageSize() {
  auto image_settings = GetImageSettings();
  return Shape(image_settings.width, image_settings.height);
}

CameraModeType PGRCamera::GetMode() {
  auto image_settings = GetImageSettings();
  return FCMode2CameraMode(image_settings.mode);
}

void PGRCamera::SetImageSizeAndMode(Shape shape, CameraModeType mode) {
  FlyCapture2::Mode fc_mode = CameraMode2FCMode(mode);
  std::lock_guard<std::mutex> guard(camera_lock_);
  CHECK_PGR(camera_.StopCapture());

  // Get fmt7 image settings
  auto image_settings = GetImageSettings();

  image_settings.mode = fc_mode;
  image_settings.height = (unsigned)shape.height;
  image_settings.width = (unsigned)shape.width;
  bool valid;
  FlyCapture2::Format7PacketInfo fmt7_packet_info;

  CHECK_PGR(camera_.ValidateFormat7Settings(&image_settings, &valid,
                                            &fmt7_packet_info));
  CHECK(valid) << "fmt7 image settings are not valid";

  CHECK_PGR(camera_.SetFormat7Configuration(
      &image_settings, fmt7_packet_info.recommendedBytesPerPacket));
  CHECK_PGR(camera_.StartCapture(PGRCamera::OnImageGrabbed, this));
}

void PGRCamera::SetPixelFormat(CameraPixelFormatType pixel_format) {
  std::lock_guard<std::mutex> guard(camera_lock_);
  CHECK_PGR(camera_.StopCapture());

  // Get fmt7 image settings
  FlyCapture2::Format7ImageSettings image_settings;
  unsigned int current_packet_size;
  float current_percentage;
  CHECK_PGR(camera_.GetFormat7Configuration(
      &image_settings, &current_packet_size, &current_percentage));

  image_settings.pixelFormat = CameraPfmt2FCPfmt(pixel_format);
  bool valid;
  FlyCapture2::Format7PacketInfo fmt7_packet_info;

  CHECK_PGR(camera_.ValidateFormat7Settings(&image_settings, &valid,
                                            &fmt7_packet_info));
  CHECK(valid) << "fmt7 image settings are not valid";

  CHECK_PGR(camera_.SetFormat7Configuration(
      &image_settings, fmt7_packet_info.recommendedBytesPerPacket));
  CHECK_PGR(camera_.StartCapture(PGRCamera::OnImageGrabbed, this));
}

void PGRCamera::SetFrameRate(float) { SAF_NOT_IMPLEMENTED; }

float PGRCamera::GetFrameRate() {
  SAF_NOT_IMPLEMENTED;
  return 0;
}

void PGRCamera::SetROI(int, int, int, int) { SAF_NOT_IMPLEMENTED; }

int PGRCamera::GetROIOffsetX() {
  SAF_NOT_IMPLEMENTED;
  return 0;
}

int PGRCamera::GetROIOffsetY() {
  SAF_NOT_IMPLEMENTED;
  return 0;
}

Shape PGRCamera::GetROIOffsetShape() {
  SAF_NOT_IMPLEMENTED;
  return Shape();
}

void PGRCamera::SetProperty(FlyCapture2::PropertyType property_type,
                            float value, bool abs, bool value_a) {
  FlyCapture2::Property prop;
  prop.type = property_type;
  prop.onOff = true;
  prop.autoManualMode = false;

  if (!abs) {
    prop.absControl = false;
    if (value_a) {
      prop.valueA = (unsigned)value;
    } else {
      prop.valueB = (unsigned)value;
    }
  } else {
    prop.absControl = true;
    prop.absValue = value;
  }

  CHECK_PGR(camera_.SetProperty(&prop));
}

float PGRCamera::GetProperty(FlyCapture2::PropertyType property_type, bool abs,
                             bool value_a) {
  //  LOG(INFO) << "Get property called";
  FlyCapture2::Property prop;
  prop.type = property_type;
  CHECK_PGR(camera_.GetProperty(&prop));

  if (abs) {
    return prop.absValue;
  } else {
    if (value_a) {
      return (float)prop.valueA;
    } else {
      return (float)prop.valueB;
    }
  }
}

CameraType PGRCamera::GetCameraType() const { return CAMERA_TYPE_PTGRAY; }

CameraModeType PGRCamera::FCMode2CameraMode(FlyCapture2::Mode fc_mode) {
  switch (fc_mode) {
    case FlyCapture2::MODE_0:
      return CAMERA_MODE_0;
    case FlyCapture2::MODE_1:
      return CAMERA_MODE_1;
    case FlyCapture2::MODE_2:
      return CAMERA_MODE_2;
    case FlyCapture2::MODE_3:
      return CAMERA_MODE_3;
    default:
      return CAMERA_MODE_INVALID;
  }
}

FlyCapture2::Mode PGRCamera::CameraMode2FCMode(CameraModeType mode) {
  switch (mode) {
    case CAMERA_MODE_0:
      return FlyCapture2::MODE_0;
    case CAMERA_MODE_1:
      return FlyCapture2::MODE_1;
    case CAMERA_MODE_2:
      return FlyCapture2::MODE_2;
    case CAMERA_MODE_3:
      return FlyCapture2::MODE_3;
    default:
      return FlyCapture2::MODE_31;
  }
}

CameraPixelFormatType PGRCamera::FCPfmt2CameraPfmt(
    FlyCapture2::PixelFormat fc_pfmt) {
  switch (fc_pfmt) {
    case FlyCapture2::PIXEL_FORMAT_RAW8:
      return CAMERA_PIXEL_FORMAT_RAW8;
    case FlyCapture2::PIXEL_FORMAT_RAW12:
      return CAMERA_PIXEL_FORMAT_RAW12;
    case FlyCapture2::PIXEL_FORMAT_BGR:
      return CAMERA_PIXEL_FORMAT_BGR;
    case FlyCapture2::PIXEL_FORMAT_411YUV8:
      return CAMERA_PIXEL_FORMAT_YUV411;
    case FlyCapture2::PIXEL_FORMAT_422YUV8:
      return CAMERA_PIXEL_FORMAT_YUV422;
    case FlyCapture2::PIXEL_FORMAT_444YUV8:
      return CAMERA_PIXEL_FORMAT_YUV444;
    case FlyCapture2::PIXEL_FORMAT_MONO8:
      return CAMERA_PIXEL_FORMAT_MONO8;
    default:
      return CAMERA_PIXEL_FORMAT_INVALID;
  }
}

FlyCapture2::PixelFormat PGRCamera::CameraPfmt2FCPfmt(
    CameraPixelFormatType pfmt) {
  switch (pfmt) {
    case CAMERA_PIXEL_FORMAT_RAW8:
      return FlyCapture2::PIXEL_FORMAT_RAW8;
    case CAMERA_PIXEL_FORMAT_RAW12:
      return FlyCapture2::PIXEL_FORMAT_RAW12;
    case CAMERA_PIXEL_FORMAT_BGR:
      return FlyCapture2::PIXEL_FORMAT_BGR;
    case CAMERA_PIXEL_FORMAT_YUV411:
      return FlyCapture2::PIXEL_FORMAT_411YUV8;
    case CAMERA_PIXEL_FORMAT_YUV422:
      return FlyCapture2::PIXEL_FORMAT_422YUV8;
    case CAMERA_PIXEL_FORMAT_YUV444:
      return FlyCapture2::PIXEL_FORMAT_444YUV8;
    case CAMERA_PIXEL_FORMAT_MONO8:
      return FlyCapture2::PIXEL_FORMAT_MONO8;
    default:
      return FlyCapture2::PIXEL_FORMAT_MONO8;  // Default to MONO8
  }
}

void PGRCamera::Reset() {
  FlyCapture2::Property prop;
  prop.onOff = true;
  prop.autoManualMode = false;
  prop.onePush = true;

  prop.type = FlyCapture2::BRIGHTNESS;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::SHARPNESS;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::GAMMA;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::GAIN;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::AUTO_EXPOSURE;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::SHUTTER;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::WHITE_BALANCE;
  prop.absControl = false;
  prop.valueA = (unsigned int)GetWBRed();
  prop.valueB = (unsigned int)GetWBBlue();
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::SATURATION;
  CHECK_PGR(camera_.SetProperty(&prop));

  prop.type = FlyCapture2::HUE;
  CHECK_PGR(camera_.SetProperty(&prop));
}
