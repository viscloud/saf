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

#ifndef SAF_COMMON_CONTEXT_H_
#define SAF_COMMON_CONTEXT_H_

#include <string.h>
#include <tinytoml/include/toml/toml.h>
#include <unordered_map>
#include <zmq.hpp>

#include "common/timer.h"
#include "utils/gst_utils.h"
#include "utils/utils.h"

const std::string H264_ENCODER_GST_ELEMENT = "h264_encoder_gst_element";
const std::string H264_DECODER_GST_ELEMENT = "h264_decoder_gst_element";
const std::string DEVICE_NUMBER = "device_number";
const std::string CONTROL_CHANNEL_NAME = "inproc://control";
const int DEVICE_NUMBER_CPU_ONLY = -1;
const int DEVICE_NUMBER_MYRIAD = -10;

/**
 * @brief Global single context, used to store and access various global
 * information.
 */
class Context {
 public:
  /**
   * @brief Get singleton instance.
   */
  static Context& GetContext() {
    static Context context;
    return context;
  }

 public:
  Context() : config_dir_("./config") {}

  void Init() {
    SetEncoderDecoderInformation();
    SetDefaultDeviceInformation();
    control_context_ = new zmq::context_t(0);
    timer_.Start();
  }

  int GetInt(const std::string& key) {
    CHECK(int_values_.count(key) != 0) << "No integer value with key  " << key;
    return int_values_[key];
  }
  double GetDouble(const std::string& key) {
    CHECK(double_values_.count(key) != 0) << "No double value with key " << key;
    return double_values_[key];
  }
  std::string GetString(const std::string& key) {
    CHECK(string_values_.count(key) != 0)
        << "No std::string value with key " << key;
    return string_values_[key];
  }
  bool GetBool(const std::string& key) {
    CHECK(bool_values_.count(key) != 0) << "No bool value with key " << key;
    return bool_values_[key];
  }

  void SetInt(const std::string& key, int value) { int_values_[key] = value; }

  void SetDouble(const std::string& key, double value) {
    double_values_[key] = value;
  }
  void SetString(const std::string& key, const std::string& value) {
    string_values_[key] = value;
  }
  void SetBool(const std::string& key, bool value) {
    bool_values_[key] = value;
  }

  Timer GetTimer() { return timer_; }

  /**
   * @brief Reload the config dir, MUST call Init() after this.
   */
  void SetConfigDir(const std::string& config_dir) { config_dir_ = config_dir; }

  std::string GetConfigDir() { return config_dir_; }

  std::string GetConfigFile(const std::string& filename) {
    return config_dir_ + "/" + filename;
  }

  zmq::context_t* GetControlContext() { return control_context_; }
  static std::string GetControlChannelName() { return CONTROL_CHANNEL_NAME; }

 private:
  std::string ValidateEncoderElement(const std::string& encoder) {
    if (IsGstElementExists(encoder)) {
      return encoder;
    } else if (IsGstElementExists("vaapih264enc")) {
      return "vaapih264enc";
    } else if (IsGstElementExists("vtenc_h264")) {
      return "vtenc_h264";
    } else if (IsGstElementExists("omxh264enc")) {
      return "omxh264enc";
    } else if (IsGstElementExists("avenc_h264_omx")) {
      return "avenc_h264_omx";
    } else if (IsGstElementExists("x264enc")) {
      return "x264enc";
    }

    LOG(WARNING) << "No known gst encoder element exists on the system";

    return "INVALID_ENCODER";
  }

  std::string ValidateDecoderElement(const std::string& decoder) {
    if (IsGstElementExists(decoder)) {
      return decoder;
    } else if (IsGstElementExists("avdec_h264")) {
      return "avdec_h264";
    } else if (IsGstElementExists("omxh264dec")) {
      return "omxh264dec";
    }

    LOG(WARNING) << "No known gst decoder element exists on the system";

    return "INVALID_DECODER";
  }
  /**
   * @brief Helper to initialze the context
   */
  void SetEncoderDecoderInformation() {
    std::string config_file = GetConfigFile("config.toml");

    auto root_value = ParseTomlFromFile(config_file);
    auto encoder_value = root_value.find("encoder");
    auto decoder_value = root_value.find("decoder");

    std::string encoder_element =
        encoder_value->get<std::string>(H264_ENCODER_GST_ELEMENT);
    std::string decoder_element =
        decoder_value->get<std::string>(H264_DECODER_GST_ELEMENT);

    std::string validated_encoder_element =
        ValidateEncoderElement(encoder_element);
    std::string validated_decoder_element =
        ValidateDecoderElement(decoder_element);

    if (validated_encoder_element != encoder_element) {
      LOG(WARNING) << "Using encoder " << validated_encoder_element
                   << " instead of " << encoder_element
                   << " from configuration";
      encoder_element = validated_encoder_element;
    }

    if (validated_decoder_element != decoder_element) {
      LOG(WARNING) << "using decoder " << validated_decoder_element
                   << " instead of " << decoder_element
                   << " from configuration";
      decoder_element = validated_decoder_element;
    }

    string_values_.insert({H264_ENCODER_GST_ELEMENT, encoder_element});
    string_values_.insert({H264_DECODER_GST_ELEMENT, decoder_element});
  }

  void SetDefaultDeviceInformation() {
    // Default, CPU only mode
    SetInt(DEVICE_NUMBER, DEVICE_NUMBER_CPU_ONLY);
  }

 private:
  std::string config_dir_;

  std::unordered_map<std::string, int> int_values_;
  std::unordered_map<std::string, std::string> string_values_;
  std::unordered_map<std::string, double> double_values_;
  std::unordered_map<std::string, bool> bool_values_;

  // Tracks time since start of Saf
  Timer timer_;

  // Control channel
  zmq::context_t* control_context_;
};

#endif  // SAF_COMMON_CONTEXT_H_
