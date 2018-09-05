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

#ifndef SAF_OPERATOR_RTSP_SENDER_H_
#define SAF_OPERATOR_RTSP_SENDER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#include "gst/app/gstappsrc.h"
#include "gst/gst.h"
#include "gst/gstbuffer.h"

#include "common/types.h"
#include "operator/operator.h"

// The GstRtspSender uses GStreamer to encode an H264 video from a image data
// stored at a specific key in the incoming frames. The resulting stream is
// then published using RTSP.
class GstRtspSender : public Operator {
 public:
  GstRtspSender(const std::string& field, const std::string& uri, int fps = 30);

  static std::shared_ptr<GstRtspSender> Create(const FactoryParamsType& params);

  // Set the Gstreamer encoder element direclty. The caller should make sure
  // that the encoder element can work on the current hardware.
  void SetEncoderElement(const std::string& encoder);

  void SetSource(StreamPtr stream);
  using Operator::SetSource;

  StreamPtr GetSink();
  using Operator::GetSink;

  static const char* kFieldKey;

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  bool CreatePipeline(int height, int width);
  std::string BuildPipelineString();
  std::string BuildCapsString(int height, int width);

  // GST Callbacks
  static void NeedDataCB(GstAppSrc* appsrc, guint size, gpointer user_data);
  static void EnoughDataCB(GstAppSrc* appsrc, gpointer userdata);

  // The frame field to encode.
  std::string field_;
  std::string uri_;
  // The framerate at which to encode. Does not change the playback rate.
  int fps_;

  // Whether the GStreamer pipeline has been created yet.
  std::atomic<bool> pipeline_created_;

  // Gst elements.
  GstBus* gst_bus_;
  GstPipeline* gst_pipeline_;
  GstAppSrc* gst_appsrc_;
  GstCaps* gst_caps_;
  GMainLoop* g_main_loop_;

  // State.
  bool need_data_;
  // Timestamp used to generate frame presentation timestamp (PTS).
  GstClockTime timestamp_;

  std::mutex lock_;
  std::string encoder_element_;
};

#endif  // SAF_OPERATOR_RTSP_SENDER_H_
