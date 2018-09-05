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

// Multi-target matcher

#include "operator/matchers/object_matcher.h"

#include "camera/camera.h"
#include "common/context.h"
#include "model/model_manager.h"
#include "operator/matchers/euclidean_matcher.h"
#include "operator/matchers/xqda_matcher.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"

ObjectMatcher::ObjectMatcher(const std::string& type, size_t batch_size,
                             float distance_threshold,
                             const ModelDesc& model_desc)
    : Operator(OPERATOR_TYPE_OBJECT_MATCHER, {}, {}),
      type_(type),
      batch_size_(batch_size),
      distance_threshold_(distance_threshold),
      model_desc_(model_desc),
      reid_is_running(true),
      reid_thread_run_(true) {
  for (size_t i = 0; i < batch_size; i++) {
    sources_.insert({GetSourceName(i), nullptr});
    sinks_.insert({GetSinkName(i), std::shared_ptr<Stream>(new Stream)});
    camera_track_buffers_.push_back(std::map<std::string, TrackInfoWeakPtr>());
  }
}

std::shared_ptr<ObjectMatcher> ObjectMatcher::Create(
    const FactoryParamsType& params) {
  auto type = params.at("type");
  auto batch_size = StringToSizet(params.at("batch_size"));
  auto distance_threshold = atof(params.at("distance_threshold").c_str());

  ModelManager& model_manager = ModelManager::GetInstance();
  auto model_name = params.at("model");
  CHECK(model_manager.HasModel(model_name));
  auto model_desc = model_manager.GetModelDesc(model_name);

  return std::make_shared<ObjectMatcher>(type, batch_size, distance_threshold,
                                         model_desc);
}

bool ObjectMatcher::Init() {
  bool result = false;
  if (type_ == "euclidean") {
    summarization_mode_ = "avg";
    matcher_ = std::make_unique<EuclideanMatcher>();
    result = matcher_->Init();
  } else if (type_ == "xqda") {
    summarization_mode_ = "max";
    matcher_ = std::make_unique<XQDAMatcher>(model_desc_);
    result = matcher_->Init();
  } else {
    LOG(FATAL) << "Matcher type " << type_ << " not supported.";
  }

  reid_thread_ = std::thread(&ObjectMatcher::ReIDThread, this);
  return result;
}

bool ObjectMatcher::OnStop() {
  if (reid_thread_.joinable()) {
    reid_thread_run_ = false;
    reid_cv_.notify_all();
    reid_thread_.join();
  }
  return true;
}

void ObjectMatcher::Process() {
  Timer timer;
  timer.Start();

  for (size_t i = 0; i < batch_size_; i++) {
    auto frame = GetFrame(GetSourceName(i));
    if (!frame) continue;

    auto camera_name = frame->GetValue<std::string>("camera_name");
    auto ids = frame->GetValue<std::vector<std::string>>("ids");
    auto timestamp =
        GetTimeSinceEpochMicros(frame->GetValue<boost::posix_time::ptime>(
            Camera::kCaptureTimeMicrosKey)) /
        1000;
    auto tags = frame->GetValue<std::vector<std::string>>("tags");
    auto features =
        frame->GetValue<std::vector<std::vector<double>>>("features");
    CHECK(ids.size() == tags.size());
    CHECK(ids.size() == features.size());

    // Search the table, get result map
    // We have 3 possiable ids
    // key id
    // mapped id
    // new id
    std::vector<std::string> mapped_ids(ids.size(), std::string());
    size_t mapped_count = 0;
    for (size_t j = 0; j < ids.size(); j++) {
      const auto& id = ids[j];
      auto it = track_buffer_.find(id);
      if (it != track_buffer_.end()) {
        mapped_ids[j] = id;
        mapped_count++;
      } else {
        // Iterate all mapped tables
        for (const auto& m : track_buffer_) {
          if (m.second->IsIDMapped(id)) {
            mapped_ids[j] = m.second->GetID();
            mapped_count++;
            break;
          }
        }
      }
    }

    // Here we got mapped result
    // We have 2 possiable ids
    // key id
    // new id
    if (mapped_count < ids.size()) {
      for (size_t j = 0; j < mapped_ids.size(); j++) {
        if (mapped_ids[j].empty()) {
          mapped_ids[j] = ids[j];
          mapped_count++;
        }
      }

      if (!reid_is_running) {
        {
          std::lock_guard<std::mutex> guard(reid_lock_);
          reid_data = std::make_unique<ReIDData>();
          reid_data->source_idx = i;
          reid_data->camera_name = camera_name;
          reid_data->ids = ids;
          reid_data->timestamp = timestamp;
          reid_data->tags = tags;
          reid_data->features = features;
        }
        reid_cv_.notify_all();
      }
    }

    CHECK(mapped_count == ids.size());
    frame->SetValue("ids", mapped_ids);
    PushFrame(GetSinkName(i), std::move(frame));
  }

  LOG(INFO) << "ObjectMatcher took " << timer.ElapsedMSec() << " ms";
}

void ObjectMatcher::ReIDThread() {
  while (reid_thread_run_) {
    // Delete TrackInfo which deactived 3600 secs
    auto now = GetTimeSinceEpochMillis();
    for (auto it = track_buffer_.begin(); it != track_buffer_.end();) {
      if (abs((long)(now - it->second->GetLastTimestamp())) > (3600 * 1000))
        track_buffer_.erase(it++);
      else
        it++;
    }

    reid_data.reset();
    reid_is_running = false;
    std::unique_lock<std::mutex> lk(reid_lock_);
    reid_cv_.wait(lk, [this] {
      // Stop waiting when reid_data available
      return reid_data || !reid_thread_run_;
    });

    if (!reid_data) continue;

    reid_is_running = true;
    auto source_idx = reid_data->source_idx;
    auto camera_name = reid_data->camera_name;
    auto ids = reid_data->ids;
    auto timestamp = reid_data->timestamp;
    auto tags = reid_data->tags;
    auto features = reid_data->features;

    std::vector<std::string> mapped_ids(ids.size(), std::string());
    size_t mapped_count = 0;

    for (auto& m : track_buffer_) {
      m.second->SetMapped(false);
    }

    // Phase 1
    for (size_t j = 0; j < ids.size(); j++) {
      const auto& id = ids[j];
      const auto& feature = features[j];
      auto it = track_buffer_.find(id);
      if (it != track_buffer_.end()) {
        mapped_ids[j] = id;
        mapped_count++;
        it->second->UpdateFeature(source_idx, timestamp, feature);
        it->second->SetMapped(true);
      } else {
        // Iterate all mapped tables
        for (const auto& m : track_buffer_) {
          if (m.second->IsIDMapped(id)) {
            mapped_ids[j] = m.second->GetID();
            mapped_count++;
            m.second->UpdateFeature(source_idx, timestamp, feature);
            CHECK(m.second->GetMapped() == false);
            m.second->SetMapped(true);
            break;
          }
        }
      }
    }

    CHECK(mapped_count < ids.size());
    if (mapped_count < ids.size()) {
      // Phase 2
      auto mapping = GetSortedMapping(ids, mapped_ids, features, source_idx);
      for (decltype(mapping.size()) j = 0; j < mapping.size(); j++) {
        auto index = std::get<0>(mapping[j]);
        auto track_info = std::get<1>(mapping[j]);
        if (mapped_ids[index].empty() && track_info->GetMapped() == false) {
          mapped_ids[index] = track_info->GetID();
          mapped_count++;
          track_info->SetIDMapped(ids[index]);
          track_info->UpdateFeature(source_idx, timestamp, features[index]);
          track_info->SetMapped(true);
        }
      }

      // Phase 3
      for (size_t j = 0; j < mapped_ids.size(); j++) {
        if (mapped_ids[j].empty()) {
          mapped_ids[j] = ids[j];
          mapped_count++;
          const auto& id = ids[j];
          const auto& tag = tags[j];
          const auto& feature = features[j];

          // Create new track into track_buffer
          auto new_track_info = std::make_shared<TrackInfo>(
              camera_name, id, tag, summarization_mode_);
          track_buffer_[id] = new_track_info;
          // Broadcast the track into network
          for (auto& m : camera_track_buffers_) {
            m[id] = new_track_info;
          }

          new_track_info->UpdateFeature(source_idx, timestamp, feature);
          new_track_info->SetMapped(true);
        }
      }
    }

    CHECK(mapped_count == ids.size());
  }
}

std::vector<std::tuple<size_t, TrackInfoPtr, double>>
ObjectMatcher::GetSortedMapping(
    const std::vector<std::string>& ids,
    const std::vector<std::string>& mapped_ids,
    const std::vector<std::vector<double>>& features,
    size_t track_buffer_index) {
  std::vector<std::tuple<size_t, TrackInfoPtr, double>> mapping;
  CHECK(ids.size() == features.size());
  CHECK(mapped_ids.size() == features.size());

  auto& camera_track_buffer = camera_track_buffers_[track_buffer_index];
  for (size_t i = 0; i < ids.size(); i++) {
    if (mapped_ids[i].empty()) {
      for (auto it = camera_track_buffer.begin();
           it != camera_track_buffer.end();) {
        if (auto track_info = it->second.lock()) {
          if (track_info->GetMapped() == false) {
            auto dist = matcher_->Match(features[i], track_info->GetFeature());
            mapping.push_back(std::make_tuple(i, track_info, dist));
          }

          it++;
        } else {
          camera_track_buffer.erase(it++);
        }
      }
    }
  }

  std::sort(mapping.begin(), mapping.end(),
            [](std::tuple<size_t, TrackInfoPtr, double>& t1,
               std::tuple<size_t, TrackInfoPtr, double>& t2) {
              auto dist1 = std::get<2>(t1);
              auto dist2 = std::get<2>(t2);
              return dist1 < dist2;
            });

  boost::optional<size_t> found;
  for (size_t i = 0; i < mapping.size(); i++) {
    auto dist = std::get<2>(mapping[i]);
    if (dist < distance_threshold_) {
      found = i;
    } else {
      break;
    }
  }

  return found ? std::vector<std::tuple<size_t, TrackInfoPtr, double>>(
                     mapping.begin(), mapping.begin() + (*found) + 1)
               : std::vector<std::tuple<size_t, TrackInfoPtr, double>>();
}
