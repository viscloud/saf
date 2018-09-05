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

#include "stream/frame.h"

#include <algorithm>
#include <regex>
#include <stdexcept>

#include <opencv2/core/core.hpp>

#include "common/types.h"

constexpr auto STOP_FRAME_KEY = "stop_frame";

const char* Frame::kFrameIdKey = "frame_id";

class FramePrinter : public boost::static_visitor<std::string> {
 public:
  std::string operator()(const double& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const float& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const int& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const long& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const unsigned long& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const bool& v) const {
    std::ostringstream output;
    output << v;
    return output.str();
  }

  std::string operator()(const boost::posix_time::ptime& v) const {
    return boost::posix_time::to_simple_string(v);
  }

  std::string operator()(const boost::posix_time::time_duration& v) const {
    return boost::posix_time::to_simple_string(v);
  }

  std::string operator()(const std::string& v) const { return v; }

  std::string operator()(const std::vector<std::string>& v) const {
    std::ostringstream output;
    output << "std::vector<std::string> = [" << std::endl;
    for (auto& s : v) {
      output << s << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<double>& v) const {
    std::ostringstream output;
    output << "std::vector<double> = [" << std::endl;
    for (auto& s : v) {
      output << s << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<Rect>& v) const {
    std::ostringstream output;
    output << "std::vector<Rect> = [" << std::endl;
    for (auto& r : v) {
      output << "Rect("
             << "px = " << r.px << "py = " << r.py << "width = " << r.width
             << "height = " << r.height << ")" << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<char>& v) const {
    std::ostringstream output;
    output << "std::vector<char>(size = " << v.size() << ") = [";
    decltype(v.size()) num_elems = v.size();
    if (num_elems > 3) {
      num_elems = 3;
    }
    for (decltype(num_elems) i = 0; i < num_elems; ++i) {
      output << +v[i] << ", ";
    }
    output << "...]";
    return output.str();
  }

  std::string operator()(const cv::Mat& v) const {
    std::ostringstream output;
    output << "cv::Mat";
    int dims = v.dims;
    if (dims <= 2) {
      std::ostringstream mout;
      for (int i = 0; i < 4 && i < v.cols * v.rows * v.channels(); ++i) {
        mout << "0x" << std::hex << (int)(v.ptr()[i]) << ", ";
      }
      output << "(rows: " << v.rows << " cols: " << v.cols
             << " channels: " << v.channels() << ") = bytes[" << mout.str()
             << "...]";
    } else {
      output << " = Unable to print because dims (" << dims << ") > 2";
    }
    return output.str();
  }

  std::string operator()(const std::vector<FaceLandmark>& v) const {
    std::ostringstream output;
    output << "std::vector<FaceLandmark> = [" << std::endl;
    for (auto& m : v) {
      output << "FaceLandmark("
             << "(" << m.x[0] << "," << m.y[0] << ")"
             << "(" << m.x[1] << "," << m.y[1] << ")"
             << "(" << m.x[2] << "," << m.y[2] << ")"
             << "(" << m.x[3] << "," << m.y[3] << ")"
             << "(" << m.x[4] << "," << m.y[4] << ")"
             << ")" << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<std::vector<float>>& v) const {
    std::ostringstream output;
    output << "std::vector<std::vector<float>> = [" << std::endl;
    for (auto& v1 : v) {
      output << "std::vector<float> = [" << std::endl;
      for (auto& f : v1) {
        output << f << std::endl;
      }
      output << "]" << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<float>& v) const {
    std::ostringstream output;
    output << "std::vector<float> = [" << std::endl;
    for (auto& s : v) {
      output << s << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<std::vector<double>>& v) const {
    std::ostringstream output;
    output << "std::vector<std::vector<double>> = [" << std::endl;
    for (auto& v1 : v) {
      output << "std::vector<double> = [" << std::endl;
      for (auto& d : v1) {
        output << d << std::endl;
      }
      output << "]" << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<Frame>& v) const {
    std::ostringstream output;
    output << "std::vector<Frame> (" << v.size() << ") = [" << std::endl;
    for (const auto& vi : v) {
      output << vi.ToString() << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::vector<int>& v) const {
    std::ostringstream output;
    output << "std::vector<int> = [" << std::endl;
    for (auto& s : v) {
      output << s << std::endl;
    }
    output << "]";
    return output.str();
  }

  std::string operator()(const std::unordered_map<int, float>& v) const {
    std::ostringstream output;
    output << "std::unordered_map<int, float> = {" << std::endl;
    for (const auto& kv_pair : v) {
      output << kv_pair.first << ": " << kv_pair.second << std::endl;
    }
    output << "}";
    return output.str();
  }

  std::string operator()(const std::unordered_map<int, bool>& v) const {
    std::ostringstream output;
    output << "std::unordered_map<int, float> = {" << std::endl;
    for (const auto& kv_pair : v) {
      output << kv_pair.first << ": " << std::boolalpha << kv_pair.second
             << std::endl;
    }
    output << "}";
    return output.str();
  }

  std::string operator()(
      const std::unordered_map<unsigned long, int>& v) const {
    std::ostringstream output;
    output << "std::unordered_map<unsigned long, int> = {" << std::endl;
    for (const auto& kv_pair : v) {
      output << kv_pair.first << ": " << kv_pair.second << std::endl;
    }
    output << "}";
    return output.str();
  }
};

class FrameJsonPrinter : public boost::static_visitor<nlohmann::json> {
 public:
  nlohmann::json operator()(const double& v) const { return v; }

  nlohmann::json operator()(const float& v) const { return v; }

  nlohmann::json operator()(const int& v) const { return v; }

  nlohmann::json operator()(const long& v) const { return v; }

  nlohmann::json operator()(const unsigned long& v) const { return v; }

  nlohmann::json operator()(const bool& v) const { return v; }

  nlohmann::json operator()(const boost::posix_time::ptime& v) const {
    return boost::posix_time::to_simple_string(v);
  }

  nlohmann::json operator()(const boost::posix_time::time_duration& v) const {
    return boost::posix_time::to_simple_string(v);
  }

  nlohmann::json operator()(const std::string& v) const { return v; }

  nlohmann::json operator()(const std::vector<std::string>& v) const {
    return v;
  }

  nlohmann::json operator()(const std::vector<double>& v) const { return v; }

  nlohmann::json operator()(const std::vector<Rect>& v) const {
    nlohmann::json j;
    for (const auto& r : v) {
      j.push_back(r.ToJson());
    }
    return j;
  }

  nlohmann::json operator()(const std::vector<char>& v) const { return v; }

  nlohmann::json operator()(const cv::Mat& v) const {
    cv::FileStorage fs(".json", cv::FileStorage::WRITE |
                                    cv::FileStorage::MEMORY |
                                    cv::FileStorage::FORMAT_JSON);
    fs << "cvMat" << v;
    std::string str = fs.releaseAndGetString();

    // There is a bug in lohmann::json::parse() for the sequence "<num>.[ ,]",
    // so replace all such sequences with "<num>[ ,]".
    std::regex bad_seq("([0-9]+)\\.([ ,])");
    str = std::regex_replace(str, bad_seq, "$1$2");

    // Convert the JSON string into a JSON object.
    return nlohmann::json::parse(str);
  }

  nlohmann::json operator()(const std::vector<float>& v) const { return v; }

  nlohmann::json operator()(const std::vector<FaceLandmark>& v) const {
    nlohmann::json j;
    for (const auto& f : v) {
      j.push_back(f.ToJson());
    }
    return j;
  }

  nlohmann::json operator()(const std::vector<std::vector<double>>& v) const {
    return v;
  }

  nlohmann::json operator()(const std::vector<std::vector<float>>& v) const {
    return v;
  }

  nlohmann::json operator()(const std::vector<Frame>& v) const {
    nlohmann::json j;
    for (const auto& vi : v) {
      j.push_back(vi.ToJson());
    }
    return j;
  }

  nlohmann::json operator()(const std::vector<int>& v) const {
    nlohmann::json j;
    for (const auto& vi : v) {
      j.push_back(vi);
    }
    return j;
  }

  nlohmann::json operator()(const std::unordered_map<int, float>& v) const {
    nlohmann::json j;
    for (const auto& kv_pair : v) {
      j[std::to_string(kv_pair.first)] = kv_pair.second;
    }
    return j;
  }

  nlohmann::json operator()(const std::unordered_map<int, bool>& v) const {
    nlohmann::json j;
    for (const auto& kv_pair : v) {
      j[std::to_string(kv_pair.first)] = kv_pair.second;
    }
    return j;
  }

  nlohmann::json operator()(
      const std::unordered_map<unsigned long, int>& v) const {
    nlohmann::json j;
    for (const auto& kv_pair : v) {
      j[std::to_string(kv_pair.first)] = kv_pair.second;
    }
    return j;
  }
};

class FrameSize : public boost::static_visitor<unsigned long> {
 public:
  unsigned long operator()(const double&) const { return sizeof(double); }

  unsigned long operator()(const float&) const { return sizeof(float); }

  unsigned long operator()(const int&) const { return sizeof(int); }

  unsigned long operator()(const long&) const { return sizeof(long); }

  unsigned long operator()(const unsigned long&) const {
    return sizeof(unsigned long);
  }

  unsigned long operator()(const bool&) const { return sizeof(bool); }

  unsigned long operator()(const boost::posix_time::ptime& time) const {
    return sizeof(time);
  }

  unsigned long operator()(
      const boost::posix_time::time_duration& duration) const {
    return sizeof(duration);
  }

  unsigned long operator()(const std::string& v) const { return v.length(); }

  unsigned long operator()(const std::vector<std::string>& v) const {
    unsigned long size_bytes = 0;
    for (const auto& s : v) {
      size_bytes += s.length();
    }
    return size_bytes;
  }

  unsigned long operator()(const std::vector<double>& v) const {
    return v.size() * sizeof(double);
  }

  unsigned long operator()(const std::vector<Rect>& v) const {
    return v.size() * sizeof(Rect);
  }

  unsigned long operator()(const std::vector<char>& v) const {
    return v.size();
  }

  unsigned long operator()(const cv::Mat& v) const {
    return v.total() * sizeof(float);
  }

  unsigned long operator()(const std::vector<float>& v) const {
    return v.size() * sizeof(float);
  }

  unsigned long operator()(const std::vector<FaceLandmark>& v) const {
    return v.size() * sizeof(FaceLandmark);
  }

  unsigned long operator()(const std::vector<std::vector<double>>& v) const {
    unsigned long size_bytes = 0;
    for (const auto& vec : v) {
      size_bytes += vec.size() * sizeof(double);
    }
    return size_bytes;
  }

  unsigned long operator()(const std::vector<std::vector<float>>& v) const {
    unsigned long size_bytes = 0;
    for (const auto& vec : v) {
      size_bytes += vec.size() * sizeof(float);
    }
    return size_bytes;
  }

  unsigned long operator()(const std::vector<Frame>& v) const {
    unsigned long size_bytes = 0;
    for (const auto& f : v) {
      size_bytes += f.GetRawSizeBytes();
    }
    return size_bytes;
  }

  unsigned long operator()(const std::vector<int>& v) const {
    return v.size() * sizeof(int);
  }

  unsigned long operator()(const std::unordered_map<int, float>& v) const {
    return v.size() * (sizeof(int) + sizeof(float));
  }

  unsigned long operator()(const std::unordered_map<int, bool>& v) const {
    return v.size() * (sizeof(int) + sizeof(bool));
  }

  unsigned long operator()(
      const std::unordered_map<unsigned long, int>& v) const {
    return v.size() * (sizeof(unsigned long) + sizeof(int));
  }
};

Frame::Frame() {}

Frame::Frame(const std::unique_ptr<Frame>& frame) : Frame(*frame) {}

Frame::Frame(const Frame& frame) : Frame(frame, {}) {}

Frame::Frame(const Frame& frame, std::unordered_set<std::string> fields) {
  flow_control_entrance_ = frame.flow_control_entrance_;
  frame_data_ = frame.frame_data_;

  bool inherit_all_fields = fields.empty();
  if (!inherit_all_fields) {
    for (auto it = frame_data_.begin(); it != frame_data_.end();) {
      if (fields.find(it->first) == fields.end()) {
        it = frame_data_.erase(it);
      } else {
        ++it;
      }
    }
  }

  // If either we are inheriting all fields or we are explicitly inheriting
  // "original_bytes", and "original_bytes" is a valid field in "frame", then
  // we need to inherit the "original_bytes" field. Doing so requires a deep
  // copy.
  auto other_it = frame.frame_data_.find("original_bytes");
  auto field_it = std::find(fields.begin(), fields.end(), "original_bytes");
  if ((inherit_all_fields || (field_it != fields.end())) &&
      (other_it != frame.frame_data_.end())) {
    CHECK(frame_data_.count("original_bytes") >= 1);
    frame_data_["original_bytes"] =
        boost::get<std::vector<char>>(other_it->second);
  }
}

void Frame::SetFlowControlEntrance(FlowControlEntrance* flow_control_entrance) {
  flow_control_entrance_ = flow_control_entrance;
}

FlowControlEntrance* Frame::GetFlowControlEntrance() {
  return flow_control_entrance_;
}

template <typename T>
T Frame::GetValue(const std::string key) const {
  auto it = frame_data_.find(key);
  if (it == frame_data_.end()) {
    std::ostringstream msg;
    msg << "Key \"" << key << "\" not in frame!";
    throw std::runtime_error(msg.str());
  }

  T val;
  try {
    val = boost::get<T>(it->second);
  } catch (boost::bad_get& e) {
    LOG(FATAL) << "Unable to get field \"" << key << " as requested type. "
               << e.what();
  }
  return val;
}

template <typename T>
void Frame::SetValue(std::string key, const T& val) {
  frame_data_[key] = val;
}

void Frame::Delete(std::string key) { frame_data_.erase(key); }

std::string Frame::ToString() const {
  FramePrinter visitor;
  std::ostringstream output;
  for (auto iter = frame_data_.begin(); iter != frame_data_.end(); iter++) {
    auto res = boost::apply_visitor(visitor, iter->second);
    output << iter->first << ": " << res << std::endl;
  }
  return output.str();
}

nlohmann::json Frame::ToJson() const {
  FrameJsonPrinter visitor;
  nlohmann::json j;
  for (const auto& e : frame_data_) {
    j[e.first] = boost::apply_visitor(visitor, e.second);
  }
  return j;
}

size_t Frame::Count(std::string key) const { return frame_data_.count(key); }

nlohmann::json Frame::GetFieldJson(const std::string& field) const {
  nlohmann::json j;
  j[field] = boost::apply_visitor(FrameJsonPrinter{}, frame_data_.at(field));
  return j;
}

std::unordered_map<std::string, Frame::field_types> Frame::GetFields() {
  return frame_data_;
}

void Frame::SetStopFrame(bool stop_frame) {
  SetValue<bool>(STOP_FRAME_KEY, stop_frame);
}

bool Frame::IsStopFrame() const {
  return Count(STOP_FRAME_KEY) && GetValue<bool>(STOP_FRAME_KEY);
}

unsigned long Frame::GetRawSizeBytes(
    std::unordered_set<std::string> fields) const {
  for (const auto& field : fields) {
    if (frame_data_.find(field) == frame_data_.end()) {
      throw std::invalid_argument("Unknown field: " + field);
    }
  }

  bool use_all_fields = fields.empty();
  FrameSize visitor;
  unsigned long size_bytes = 0;
  for (auto it = frame_data_.begin(); it != frame_data_.end(); ++it) {
    std::string field = it->first;
    if (use_all_fields || fields.find(field) != fields.end()) {
      size_bytes += boost::apply_visitor(visitor, frame_data_.at(field));
    }
  }
  return size_bytes;
}

// Types declared in Field
template void Frame::SetValue(std::string, const double&);
template void Frame::SetValue(std::string, const float&);
template void Frame::SetValue(std::string, const int&);
template void Frame::SetValue(std::string, const long&);
template void Frame::SetValue(std::string, const unsigned long&);
template void Frame::SetValue(std::string, const bool&);
template void Frame::SetValue(std::string, const boost::posix_time::ptime&);
template void Frame::SetValue(std::string,
                              const boost::posix_time::time_duration&);
template void Frame::SetValue(std::string, const std::string&);
template void Frame::SetValue(std::string, const std::vector<std::string>&);
template void Frame::SetValue(std::string, const std::vector<double>&);
template void Frame::SetValue(std::string, const std::vector<Rect>&);
template void Frame::SetValue(std::string, const std::vector<char>&);
template void Frame::SetValue(std::string, const cv::Mat&);
template void Frame::SetValue(std::string, const std::vector<FaceLandmark>&);
template void Frame::SetValue(std::string,
                              const std::vector<std::vector<float>>&);
template void Frame::SetValue(std::string, const std::vector<float>&);
template void Frame::SetValue(std::string,
                              const std::vector<std::vector<double>>&);
template void Frame::SetValue(std::string, const std::vector<Frame>&);
template void Frame::SetValue(std::string, const std::vector<int>&);
template void Frame::SetValue(std::string,
                              const std::unordered_map<int, float>&);
template void Frame::SetValue(std::string,
                              const std::unordered_map<int, bool>&);
template void Frame::SetValue(std::string,
                              const std::unordered_map<unsigned long, int>&);

template double Frame::GetValue(std::string) const;
template float Frame::GetValue(std::string) const;
template int Frame::GetValue(std::string) const;
template long Frame::GetValue(std::string) const;
template unsigned long Frame::GetValue(std::string) const;
template bool Frame::GetValue(std::string) const;
template boost::posix_time::ptime Frame::GetValue(std::string) const;
template boost::posix_time::time_duration Frame::GetValue(std::string) const;
template std::string Frame::GetValue(std::string) const;
template std::vector<std::string> Frame::GetValue(std::string) const;
template std::vector<double> Frame::GetValue(std::string) const;
template std::vector<Rect> Frame::GetValue(std::string) const;
template cv::Mat Frame::GetValue(std::string) const;
template std::vector<char> Frame::GetValue(std::string) const;
template std::vector<FaceLandmark> Frame::GetValue(std::string) const;
template std::vector<std::vector<float>> Frame::GetValue(std::string) const;
template std::vector<float> Frame::GetValue(std::string) const;
template std::vector<std::vector<double>> Frame::GetValue(std::string) const;
template std::vector<Frame> Frame::GetValue(std::string) const;
template std::vector<int> Frame::GetValue(std::string) const;
template std::unordered_map<int, float> Frame::GetValue(std::string) const;
template std::unordered_map<int, bool> Frame::GetValue(std::string) const;
template std::unordered_map<unsigned long, int> Frame::GetValue(
    std::string) const;
