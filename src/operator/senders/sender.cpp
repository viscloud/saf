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

// Send metadata to remote endpoint

#include "operator/senders/sender.h"
#include <boost/archive/binary_oarchive.hpp>

#include <chrono>
#include <iostream>

#include "camera/camera.h"
#ifdef USE_KAFKA
#include "operator/senders/kafka_sender.h"
#endif  // USE_KAFKA
#ifdef USE_MQTT
#include "operator/senders/mqtt_sender.h"
#endif  // USE_MQTT
#ifdef USE_WEBSOCKET
#include "operator/senders/websocket_sender.h"
#endif  // USE_WEBSOCKET
#include "saf.pb.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

Sender::Sender(const std::string& endpoint, const std::string& package_type,
               size_t batch_size)
    : Operator(OPERATOR_TYPE_SENDER, {}, {}),
      endpoint_(endpoint),
      package_type_(package_type),
      batch_size_(batch_size) {
  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({GetSourceName(i), nullptr});
  }
}

std::shared_ptr<Sender> Sender::Create(const FactoryParamsType& params) {
  auto endpoint = params.at("endpoint");
  auto package_type = params.at("package_type");
  auto batch_size = StringToSizet(params.at("batch_size"));
  return std::make_shared<Sender>(endpoint, package_type, batch_size);
}

bool Sender::Init() {
  bool result = false;
  if (!endpoint_.empty()) {
#ifdef USE_WEBSOCKET
    if (strncmp(endpoint_.c_str(), "ws://", 5) == 0) {
      sender_.reset(new WebsocketSender(endpoint_));
      result = sender_->Init();
    } else
#endif  // USE_WEBSOCKET
#ifdef USE_KAFKA
        if (strncmp(endpoint_.c_str(), "kafka://", 8) == 0) {
      sender_.reset(new KafkaSender(endpoint_));
      result = sender_->Init();
    } else
#endif  // USE_KAFKA
#ifdef USE_MQTT
        if (strncmp(endpoint_.c_str(), "mqtt://", 7) == 0) {
      sender_.reset(new MqttSender(endpoint_));
      result = sender_->Init();
    } else
#endif  // USE_MQTT
    {
      LOG(FATAL) << "Sender type not supported.";
    }
  } else {
    LOG(FATAL) << "Sender endpoint cannot be empty.";
  }
  return result;
}

bool Sender::OnStop() {
  sender_ = nullptr;
  return true;
}

void Sender::Process() {
  for (size_t i = 0; i < batch_size_; i++) {
    auto frame = GetFrame(GetSourceName(i));
    if (!frame) continue;

    if (!endpoint_.empty()) Send(std::move(frame));
  }
}

void Sender::Send(std::unique_ptr<Frame> frame) {
  auto camera_name = frame->GetValue<std::string>("camera_name");
  auto image = frame->GetValue<cv::Mat>("original_image");
  // Convert micros to millis
  auto timestamp =
      GetTimeSinceEpochMicros(frame->GetValue<boost::posix_time::ptime>(
          Camera::kCaptureTimeMicrosKey)) /
      1000;
  auto frame_id = frame->GetValue<unsigned long>("frame_id");

  std::stringstream ss;
  if (package_type_ == "thumbnails") {
    auto tags = frame->GetValue<std::vector<std::string>>("tags");
    auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    DetectionProto info;
    info.set_capture_time_micros(std::to_string(timestamp));
    info.set_stream_id(camera_name);
    info.set_frame_id(frame_id);
    CHECK(tags.size() == bboxes.size());
    for (decltype(bboxes.size()) i = 0; i < bboxes.size(); ++i) {
      cv::Rect r(bboxes[i].px, bboxes[i].py, bboxes[i].width, bboxes[i].height);
      cv::Mat image_cv(image, r);
      std::vector<uchar> image_bin;
      imencode(".jpg", image_cv, image_bin);
      std::string image_bin_str(image_bin.begin(), image_bin.end());

      auto th = info.add_thumbnails();
      th->set_thumbnail(image_bin_str);
      th->set_label(tags[i]);
      if (frame->Count("ids") > 0) {
        auto ids = frame->GetValue<std::vector<std::string>>("ids");
        CHECK(ids.size() == bboxes.size());
        th->set_id(ids[i]);
      }
      if (frame->Count("features") > 0) {
        auto features =
            frame->GetValue<std::vector<std::vector<double>>>("features");
        CHECK(features.size() == bboxes.size());
        auto feature = th->mutable_feature();
        for (const auto& m : features[i]) feature->add_feature(m);
      }
    }

    info.SerializeToOstream(&ss);
  } else if (package_type_ == "frame") {
    FrameProto info;
    info.set_capture_time_micros(std::to_string(timestamp));
    info.set_stream_id(camera_name);
    info.set_frame_id(frame_id);
#ifdef SR_USE_BOOST_ARCHIVE
    std::stringstream image_string;
    try {
      boost::archive::binary_oarchive ar(image_string);
      ar << image;
    } catch (const boost::archive::archive_exception& e) {
      LOG(INFO) << "Boost serialization error: " << e.what();
    }
    info.set_image(image_string.str());
#else
    std::vector<uchar> image_bin;
    imencode(".jpg", image, image_bin);
    std::string image_bin_str(image_bin.begin(), image_bin.end());
    info.set_image(image_bin_str);
#endif

    if (frame->Count("bounding_boxes") > 0) {
      auto tags = frame->GetValue<std::vector<std::string>>("tags");
      auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
      for (decltype(bboxes.size()) i = 0; i < bboxes.size(); ++i) {
        auto ri = info.add_rect_infos();
        auto bb = ri->mutable_bbox();
        bb->set_x(bboxes[i].px);
        bb->set_y(bboxes[i].py);
        bb->set_w(bboxes[i].width);
        bb->set_h(bboxes[i].height);
        ri->set_label(tags[i]);
        if (frame->Count("ids") > 0) {
          auto ids = frame->GetValue<std::vector<std::string>>("ids");
          CHECK(ids.size() == bboxes.size());
          ri->set_id(ids[i]);
        }
        if (frame->Count("features") > 0) {
          auto features =
              frame->GetValue<std::vector<std::vector<double>>>("features");
          CHECK(features.size() == bboxes.size());
          auto feature = ri->mutable_feature();
          for (const auto& m : features[i]) feature->add_feature(m);
        }
      }
    }

    info.SerializeToOstream(&ss);
  } else {
    LOG(FATAL) << "Package type " << package_type_ << " not supported";
  }

  sender_->Send(ss.str());  // NOTE: Sending redundant msgs for the time being
  sender_->Send(ss.str(), camera_name);
}
