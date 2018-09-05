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

// Receive metadata from remote endpoint

#include "operator/receivers/receiver.h"
#include <boost/archive/binary_iarchive.hpp>

#include <chrono>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "camera/camera.h"
#ifdef USE_MQTT
#include "operator/receivers/mqtt_receiver.h"
#endif  // USE_MQTT
#ifdef USE_KAFKA
#include "operator/receivers/kafka_receiver.h"
#endif  // USE_KAFKA
#ifdef USE_WEBSOCKET
#include "operator/receivers/websocket_receiver.h"
#endif  // USE_WEBSOCKET
#include "saf.pb.h"

Receiver::Receiver(const std::string& endpoint, const std::string& package_type,
                   const std::string& aux)
    : Operator(OPERATOR_TYPE_RECEIVER, {}, {GetSinkName()}),
      aux_(aux),
      endpoint_(endpoint),
      package_type_(package_type) {}

std::shared_ptr<Receiver> Receiver::Create(const FactoryParamsType& params) {
  auto endpoint = params.at("endpoint");
  auto package_type = params.at("package_type");
  return std::make_shared<Receiver>(endpoint, package_type);
}

bool Receiver::Init() {
  bool result = true;
  if (!endpoint_.empty()) {
#ifdef USE_MQTT
    if (strncmp(endpoint_.c_str(), "mqtt://", 7) == 0) {
      receiver_ = std::make_unique<MqttReceiver>(endpoint_, aux_);
      result = receiver_->Init();
    } else
#endif  // USE_MQTT
#ifdef USE_WEBSOCKET
        if (strncmp(endpoint_.c_str(), "ws://", 5) == 0) {
      receiver_ = std::make_unique<WebsocketReceiver>(endpoint_);
      result = receiver_->Init();
    } else
#endif  // USE_WEBSOCKET
#ifdef USE_KAFKA
        if (strncmp(endpoint_.c_str(), "kafka://", 8) == 0) {
      receiver_ = std::make_unique<KafkaReceiver>(endpoint_);
      result = receiver_->Init();
    } else
#endif  // USE_KAFKA
    {
      LOG(FATAL) << "Receiver type not supported.";
    }
  }
  return result;
}

bool Receiver::OnStop() { return true; }

void Receiver::Process() {
  auto in_ss = receiver_->Receive(aux_);

  if (in_ss) {
    if (package_type_ == "thumbnails") {
      SAF_NOT_IMPLEMENTED;
    } else if (package_type_ == "frame") {
      FrameProto info;
      if (info.ParseFromIstream(&(*in_ss))) {
        auto stream_id = info.stream_id();
        auto frame_id = info.frame_id();
        auto image_str = info.image();
#ifdef SR_USE_BOOST_ARCHIVE
        std::stringstream image_string(image_str);
        cv::Mat image;
        try {
          boost::archive::binary_iarchive ar(image_string);
          ar >> image;
        } catch (const boost::archive::archive_exception& e) {
          LOG(INFO) << "Boost serialization error: " << e.what();
        }
#else
        std::vector<char> vec_data(&image_str[0],
                                   &image_str[0] + image_str.size());
        cv::Mat image = cv::imdecode(vec_data, 1);
#endif
        std::vector<Rect> bboxes;
        std::vector<std::string> tags;
        std::vector<std::string> ids;
        std::vector<std::vector<double>> features;
        for (int i = 0; i < info.rect_infos_size(); ++i) {
          const FrameProto_RectInfo& fri = info.rect_infos(i);
          const FrameProto_Rect fr = fri.bbox();
          int x = fr.x();
          int y = fr.y();
          int w = fr.w();
          int h = fr.h();
          Rect r(x, y, w, h);
          bboxes.push_back(r);
          auto l = fri.label();
          tags.push_back(l);
          if (fri.has_id()) {
            ids.push_back(fri.id());
          }
          if (fri.has_feature()) {
            std::vector<double> _f;
            auto fe = fri.feature();
            for (int j = 0; j < fe.feature_size(); j++) {
              _f.push_back(fe.feature(j));
            }
            features.push_back(_f);
          }
        }

        auto frame = std::make_unique<Frame>();
        frame->SetValue("frame_id", frame_id);
        frame->SetValue("camera_name", stream_id);
        frame->SetValue("bounding_boxes", bboxes);
        frame->SetValue("tags", tags);
        frame->SetValue(Camera::kCaptureTimeMicrosKey,
                        boost::posix_time::microsec_clock::local_time());

        const cv::Mat& pixels = image;
        frame->SetValue(
            "original_bytes",
            std::vector<char>(
                (char*)pixels.data,
                (char*)pixels.data + pixels.total() * pixels.elemSize()));
        frame->SetValue("original_image", pixels);

        if (ids.size() > 0) {
          frame->SetValue("ids", ids);
        }
        if (features.size() > 0) {
          frame->SetValue("features", features);
        }
        PushFrame(GetSinkName(), std::move(frame));
      } else {
        LOG(ERROR) << "Failed to parse FrameProto.";
      }
    } else {
      LOG(FATAL) << "Package type " << package_type_ << " not supported";
    }
  }
}
