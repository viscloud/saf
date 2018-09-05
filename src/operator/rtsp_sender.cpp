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

#include "operator/rtsp_sender.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <opencv2/opencv.hpp>

#include "common/context.h"
#include "utils/string_utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

const char* GstRtspSender::kFieldKey = "GstRtspSender.field";

GstRtspSender::GstRtspSender(const std::string& field, const std::string& uri,
                             int fps)
    : Operator(OPERATOR_TYPE_ENCODER, {SOURCE_NAME}, {SINK_NAME}),
      field_(field),
      uri_(uri),
      fps_(fps),
      pipeline_created_(false),
      need_data_(false),
      timestamp_(0) {
  if (fps <= 0) {
    std::ostringstream msg;
    msg << "Fps must be greater than 0, but is: " << fps;
    throw std::invalid_argument(msg.str());
  }
  SetEncoderElement(Context::GetContext().GetString(H264_ENCODER_GST_ELEMENT));
}

std::shared_ptr<GstRtspSender> GstRtspSender::Create(const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

void GstRtspSender::SetEncoderElement(const std::string& encoder) {
  encoder_element_ = encoder;
}

void GstRtspSender::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr GstRtspSender::GetSink() { return Operator::GetSink(SINK_NAME); }

bool GstRtspSender::Init() { return true; }

bool GstRtspSender::OnStop() {
  std::lock_guard<std::mutex> guard(lock_);

  need_data_ = false;
  VLOG(1) << "Stopping RTSP sender pipeline.";

  if (pipeline_created_) {
    gst_app_src_end_of_stream(gst_appsrc_);

    const int WAIT_UNTIL_EOS_SENT_MS = 200;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(WAIT_UNTIL_EOS_SENT_MS));

    GstStateChangeReturn ret =
        gst_element_set_state(GST_ELEMENT(gst_pipeline_), GST_STATE_NULL);

    if (ret != GST_STATE_CHANGE_SUCCESS) {
      LOG(ERROR) << "GStreamer failed to stop the RTSP sender pipeline.";
    }
  }

  VLOG(1) << "RTSP sender pipeline stopped.";
  return true;
}

void GstRtspSender::Process() {
  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);
  cv::Mat img = frame->GetValue<cv::Mat>(field_);
  {
    std::lock_guard<std::mutex> guard(lock_);

    if (!pipeline_created_) {
      cv::Size size = img.size();
      if (!CreatePipeline(size.height, size.width)) {
        throw std::runtime_error("Unable to create encoder pipeline!");
      }
      pipeline_created_ = true;
    }

    if (!need_data_) {
      return;
    }

    // Give PTS to the buffer
    int frame_size_bytes = img.total() * img.elemSize();
    GstMapInfo info;
    GstBuffer* buffer =
        gst_buffer_new_allocate(nullptr, frame_size_bytes, nullptr);
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    // Copy the image to gst buffer, should have better way such as using
    // gst_buffer_new_wrapper_full(). TODO
    memcpy(info.data, img.data, frame_size_bytes);
    gst_buffer_unmap(buffer, &info);

    GST_BUFFER_PTS(buffer) = timestamp_;

    // TODO: FPS is fixed right now
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, fps_);
    timestamp_ += GST_BUFFER_DURATION(buffer);

    GstFlowReturn ret;
    g_signal_emit_by_name(gst_appsrc_, "push-buffer", buffer, &ret);

    gst_buffer_unref(buffer);

    if (ret != 0) {
      LOG(WARNING) << "Unable to push frame to encoder stream (code: " << ret
                   << ")";
    }

    // Poll messages from the bus
    while (true) {
      GstMessage* msg = gst_bus_pop(gst_bus_);

      if (!msg) {
        break;
      }

      VLOG(1) << "Got message of type: " << GST_MESSAGE_TYPE_NAME(msg);

      switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
          VLOG(1) << "End of stream encountered";
          break;
        case GST_MESSAGE_ERROR: {
          gchar* debug;
          GError* error;

          gst_message_parse_error(msg, &error, &debug);
          g_free(debug);

          LOG(ERROR) << "GST error: " << error->message;
          g_error_free(error);
          break;
        }
        case GST_MESSAGE_WARNING: {
          gchar* debug;
          GError* error;
          gst_message_parse_warning(msg, &error, &debug);
          g_free(debug);

          LOG(WARNING) << "GST warning: " << error->message;
          g_error_free(error);
          break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
          GstState old_state, new_state;

          gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
          VLOG(1) << "Element " << GST_OBJECT_NAME(msg->src)
                  << " changed state from "
                  << gst_element_state_get_name(old_state) << " to "
                  << gst_element_state_get_name(new_state);
          break;
        }
        case GST_MESSAGE_STREAM_STATUS:
          GstStreamStatusType status;
          gst_message_parse_stream_status(msg, &status, NULL);
          switch (status) {
            case GST_STREAM_STATUS_TYPE_CREATE:
              VLOG(1) << "Stream created";
              break;
            case GST_STREAM_STATUS_TYPE_ENTER:
              VLOG(1) << "Stream entered";
              break;
            default:
              VLOG(1) << "Other stream status: " << status;
          }
          break;
        default:
          break;
      }

      gst_message_unref(msg);
    }
  }

  frame->SetValue(kFieldKey, field_);
  PushFrame(SINK_NAME, std::move(frame));
}

bool GstRtspSender::CreatePipeline(int height, int width) {
  g_main_loop_ = g_main_loop_new(NULL, FALSE);

  // Build Gst pipeline
  GError* err = nullptr;
  std::string pipeline_str = BuildPipelineString();

  gst_pipeline_ = GST_PIPELINE(gst_parse_launch(pipeline_str.c_str(), &err));
  if (err != nullptr) {
    LOG(ERROR) << "gstreamer failed to launch pipeline: " << pipeline_str;
    LOG(ERROR) << err->message;
    g_error_free(err);
    return false;
  }

  if (gst_pipeline_ == nullptr) {
    LOG(ERROR) << "Failed to convert gst_element to gst_pipeline";
    return false;
  }

  gst_bus_ = gst_pipeline_get_bus(gst_pipeline_);
  if (gst_bus_ == nullptr) {
    LOG(ERROR) << "Failed to retrieve gst_bus from gst_pipeline";
    return false;
  }

  // Get the appsrc and connect callbacks
  GstElement* appsrc_element =
      gst_bin_get_by_name(GST_BIN(gst_pipeline_), "GstRtspSender");
  GstAppSrc* appsrc = GST_APP_SRC(appsrc_element);

  if (appsrc == nullptr) {
    LOG(ERROR) << "Failed to get appsrc from pipeline";
    return false;
  }

  gst_appsrc_ = appsrc;

  // Set the caps of the appsrc
  std::string caps_str = BuildCapsString(height, width);
  gst_caps_ = gst_caps_from_string(caps_str.c_str());

  if (gst_caps_ == nullptr) {
    LOG(ERROR) << "Failed to parse caps from caps string";
    return false;
  }

  gst_app_src_set_caps(gst_appsrc_, gst_caps_);
  g_object_set(G_OBJECT(gst_appsrc_), "stream-type", 0, "format",
               GST_FORMAT_TIME, NULL);

  GstAppSrcCallbacks callbacks;
  callbacks.enough_data = GstRtspSender::EnoughDataCB;
  callbacks.need_data = GstRtspSender::NeedDataCB;
  callbacks.seek_data = NULL;
  gst_app_src_set_callbacks(gst_appsrc_, &callbacks, (void*)this, NULL);

  // Play the pipeline
  GstStateChangeReturn result =
      gst_element_set_state(GST_ELEMENT(gst_pipeline_), GST_STATE_PLAYING);

  if (result != GST_STATE_CHANGE_ASYNC && result != GST_STATE_CHANGE_SUCCESS) {
    LOG(ERROR) << "Can't start gst pipeline";
    return false;
  }

  VLOG(1) << "Pipeline launched";
  return true;
}

std::string GstRtspSender::BuildPipelineString() {
  std::ostringstream pipeline;
  pipeline << "appsrc name=GstRtspSender ! "
           << "videoconvert ! " << encoder_element_
           << " ! video/x-h264 ! rtspclientsink latency=0 location=" << uri_;
  std::string pipeline_s = pipeline.str();
  LOG(INFO) << "RTSP sender pipeline: " << pipeline_s;
  return pipeline_s;
}

std::string GstRtspSender::BuildCapsString(int height, int width) {
  std::ostringstream ss;
  CHECK(fps_ > 0) << "RtspSender expects fps > 0";
  ss << "video/x-raw,format=(string)BGR,width=" << width << ",height=" << height
     << ",framerate=(fraction)" << fps_ << "/1";
  return ss.str();
}

void GstRtspSender::NeedDataCB(GstAppSrc*, guint, gpointer user_data) {
  if (user_data == nullptr) {
    return;
  }

  GstRtspSender* encoder = (GstRtspSender*)user_data;
  if (encoder->IsStarted()) encoder->need_data_ = true;
}

void GstRtspSender::EnoughDataCB(GstAppSrc*, gpointer user_data) {
  VLOG(1) << "Received enough data signal";
  if (!user_data) {
    return;
  }

  ((GstRtspSender*)user_data)->need_data_ = false;
}
