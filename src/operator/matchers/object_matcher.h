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

#ifndef SAF_OPERATOR_MATCHERS_OBJECT_MATCHER_H_
#define SAF_OPERATOR_MATCHERS_OBJECT_MATCHER_H_

#include <boost/circular_buffer.hpp>
#include <boost/optional.hpp>

#include "camera/camera.h"
#include "common/context.h"
#include "model/model.h"
#include "operator/operator.h"

class TrackInfo {
 public:
  TrackInfo(const std::string& camera_name, const std::string& id,
            const std::string& tag, const std::string& summarization_mode)
      : camera_name_(camera_name),
        id_(id),
        tag_(tag),
        mapped_(false),
        summarization_mode_(summarization_mode) {
    features_.set_capacity(30);
  }

  void UpdateFeature(int source_idx, unsigned long last_timestamp,
                     const std::vector<double>& feature) {
    source_idx_ = source_idx;
    last_timestamp_ = last_timestamp;
    features_.push_back(feature);

    // SummarizeTrackFeature
    if (summarization_mode_.compare("avg") == 0) {
      summarized_feature_ = std::vector<double>(feature.size(), 0.f);
      for (const auto& m : features_) {
        for (size_t i = 0; i < m.size(); i++) {
          summarized_feature_[i] += m[i];
        }
      }
      for (auto& m : summarized_feature_) {
        m /= features_.size();
      }
    } else if (summarization_mode_.compare("max") == 0) {
      summarized_feature_ = std::vector<double>(
          feature.size(), std::numeric_limits<double>::min());
      for (const auto& m : features_) {
        for (size_t i = 0; i < m.size(); i++) {
          if (m[i] > summarized_feature_[i]) {
            summarized_feature_[i] = m[i];
          }
        }
      }
    } else {
      LOG(FATAL) << "Matcher summarization mode " << summarization_mode_
                 << " not supported.";
    }
  }

  const std::vector<double>& GetFeature() const { return summarized_feature_; }
  std::string GetID() const { return id_; }
  bool GetMapped() const { return mapped_; }
  void SetMapped(bool mapped) { mapped_ = mapped; }
  void SetIDMapped(const std::string& id) { mapped_ids_.insert(id); }
  bool IsIDMapped(const std::string& id) const {
    return mapped_ids_.find(id) != mapped_ids_.end();
  }
  unsigned long GetLastTimestamp() { return last_timestamp_; }

 private:
  std::string camera_name_;
  std::string id_;
  std::string tag_;
  int source_idx_;
  unsigned long last_timestamp_;
  boost::circular_buffer<std::vector<double>> features_;
  std::vector<double> summarized_feature_;
  bool mapped_;
  std::string summarization_mode_;
  std::set<std::string> mapped_ids_;
};

class BaseMatcher {
 public:
  BaseMatcher() {}
  virtual ~BaseMatcher() {}
  virtual bool Init() = 0;
  virtual double Match(const std::vector<double>& feat1,
                       const std::vector<double>& feat2) = 0;
};

typedef std::shared_ptr<TrackInfo> TrackInfoPtr;
typedef std::weak_ptr<TrackInfo> TrackInfoWeakPtr;
class ObjectMatcher : public Operator {
 public:
  struct ReIDData {
    int source_idx;
    std::string camera_name;
    std::vector<std::string> ids;
    unsigned long timestamp;
    std::vector<std::string> tags;
    std::vector<std::vector<double>> features;
  };
  ObjectMatcher(const std::string& type, size_t batch_size,
                float distance_threshold, const ModelDesc& model_desc);
  static std::shared_ptr<ObjectMatcher> Create(const FactoryParamsType& params);
  static std::string GetSourceName(int index) {
    return "input" + std::to_string(index);
  }
  static std::string GetSinkName(int index) {
    return "output" + std::to_string(index);
  }

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  void ReIDThread();
  std::vector<std::tuple<size_t, TrackInfoPtr, double>> GetSortedMapping(
      const std::vector<std::string>& ids,
      const std::vector<std::string>& mapped_ids,
      const std::vector<std::vector<double>>& features,
      size_t track_buffer_index);

 private:
  std::string type_;
  size_t batch_size_;
  float distance_threshold_;
  std::string summarization_mode_;
  ModelDesc model_desc_;
  std::unique_ptr<BaseMatcher> matcher_;
  std::map<std::string, TrackInfoPtr> track_buffer_;  // <id, TrackInfoPtr>
  std::vector<std::map<std::string, TrackInfoWeakPtr>>
      camera_track_buffers_;  // <camera_name, <id, TrackInfoWeakPtr>>
  std::unique_ptr<ReIDData> reid_data;
  bool reid_is_running;
  std::mutex reid_lock_;
  std::condition_variable reid_cv_;
  std::thread reid_thread_;
  bool reid_thread_run_;
};

#endif  // SAF_OPERATOR_MATCHERS_OBJECT_MATCHER_H_
