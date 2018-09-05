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

#include "operator/operator.h"

#include <sstream>
#include <stdexcept>

#include "camera/camera.h"
#include "common/types.h"
#include "utils/utils.h"

static const size_t SLIDING_WINDOW_SIZE = 25;

Operator::Operator(OperatorType type,
                   const std::vector<std::string>& source_names,
                   const std::vector<std::string>& sink_names)
    : num_frames_processed_(0),
      avg_processing_latency_ms_(0),
      processing_latencies_sum_ms_(0),
      trailing_avg_processing_latency_ms_(0),
      queue_latency_sum_ms_(0),
      type_(type),
      block_on_push_(false) {
  found_last_frame_ = false;
  stopped_ = true;

  for (const auto& source_name : source_names) {
    sources_.insert({source_name, nullptr});
    source_frame_cache_[source_name] = nullptr;
  }

  for (const auto& sink_name : sink_names) {
    sinks_.insert({sink_name, StreamPtr(new Stream)});
  }

  control_socket_ =
      new zmq::socket_t(*Context::GetContext().GetControlContext(), ZMQ_PUSH);
  // XXX: PUSH sockets can only send
  control_socket_->connect(Context::GetControlChannelName());
  int linger = 0;
  control_socket_->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
}

Operator::~Operator() {
  control_socket_->close();
  delete control_socket_;
}

void Operator::SetSink(const std::string& name, StreamPtr stream) {
  sinks_[name] = stream;
}

StreamPtr Operator::GetSink(const std::string& name) {
  if (sinks_.find(name) == sinks_.end()) {
    std::ostringstream msg;
    msg << "Sink \"" << name << "\" does not exist for operator \""
        << GetStringForOperatorType(GetType()) << "\". Available sinks: ";
    for (const auto& s : sinks_) {
      msg << s.first << " ";
    }
    throw std::runtime_error(msg.str());
  }
  return sinks_.at(name);
}

void Operator::SetSource(const std::string& name, StreamPtr stream) {
  if (sources_.find(name) == sources_.end()) {
    std::ostringstream msg;
    msg << "Source \"" << name << "\" does not exist for operator \""
        << GetStringForOperatorType(GetType()) << "\". Available sources: ";
    for (const auto& s : sources_) {
      msg << s.first << " ";
    }
    throw std::runtime_error(msg.str());
  }
  sources_[name] = stream;
}

bool Operator::Start(size_t buf_size) {
  LOG(INFO) << "Starting " << GetName() << "...";
  CHECK(stopped_) << "Operator " << GetName() << " has already started";

  op_timer_.Start();

  // Check sources are filled
  for (const auto& source : sources_) {
    CHECK(source.second != nullptr)
        << "Source \"" << source.first << "\" is not set.";
  }

  // Subscribe sources
  for (auto& source : sources_) {
    readers_.emplace(source.first, source.second->Subscribe(buf_size));
  }

  stopped_ = false;
  process_thread_ = std::thread(&Operator::OperatorLoop, this);
  return true;
}

bool Operator::Stop() {
  LOG(INFO) << "Stopping " << GetName() << "...";
  if (stopped_) {
    LOG(WARNING) << "Stop() called on a Operator that was already stopped!";
    return true;
  }

  // This signals that the process thread should stop processing new frames.
  stopped_ = true;

  // Stop all sink streams, which wakes up any blocking calls to
  // Stream::PushFrame() in the process thread.
  for (const auto& p : sinks_) {
    p.second->Stop();
  }

  // Unsubscribe from the source streams, which wakes up any blocking calls to
  // StreamReader::PopFrame() in the process thread.
  for (const auto& reader : readers_) {
    reader.second->UnSubscribe();
  }

  // Join the process thread, completing the main processing loop.
  process_thread_.join();

  // Do any operator-specific cleanup.
  bool result = OnStop();

  // Deallocate the source StreamReaders.
  readers_.clear();

  LOG(INFO) << "Stopped " << GetName();
  return result;
}

void Operator::OperatorLoopDirect() {
  CHECK(Init()) << "Operator is not able to be initialized";
  while (!stopped_ && !found_last_frame_) {
    Process();
    ++num_frames_processed_;
  }
}

void Operator::OperatorLoop() {
  CHECK(Init()) << "Operator " << GetStringForOperatorType(type_)
                << " is not able to be initialized";
  Timer local_timer;
  while (!stopped_ && !found_last_frame_) {
    // Cache source frames
    source_frame_cache_.clear();
    for (auto& reader : readers_) {
      auto source_name = reader.first;
      auto source_stream = reader.second;

      auto frame = source_stream->PopFrame(15);
      if (frame == nullptr) {
        // This is for nonblock feature.
        // Case 1: Stop() was called on the StreamReader.
        // Case 2: StreamReader timeout.
        // Therefore, we should continue to read other readers.
        continue;
      } else if (frame->IsStopFrame()) {
        // This frame is signaling the pipeline to stop. We need to forward
        // it to our sinks, then not process it or any future frames.
        for (const auto& p : sinks_) {
          PushFrame(p.first, std::make_unique<Frame>(frame));
        }
        return;
      } else {
        // Calculate queue latency
        auto start_micros = frame->GetValue<boost::posix_time::ptime>(
            Camera::kCaptureTimeMicrosKey);
        boost::posix_time::ptime end_micros =
            boost::posix_time::microsec_clock::local_time();
        queue_latency_sum_ms_ +=
            (end_micros - start_micros).total_milliseconds();
        source_frame_cache_[source_name] = std::move(frame);
      }
    }

    // This is for nonblock feature.
    // Camera operator has no readers, should skip this.
    if (!readers_.empty() && source_frame_cache_.empty()) {
      // Other operator should continue the while loop when nothing received.
      continue;
    }

    processing_start_micros_ = boost::posix_time::microsec_clock::local_time();
    Process();
    double processing_latency_ms =
        (double)(boost::posix_time::microsec_clock::local_time() -
                 processing_start_micros_)
            .total_microseconds();
    processing_start_micros_ = boost::posix_time::not_a_date_time;

    ++num_frames_processed_;

    // Update average processing latency.
    avg_processing_latency_ms_ =
        (avg_processing_latency_ms_ * (num_frames_processed_ - 1) +
         processing_latency_ms) /
        num_frames_processed_;

    // Update trailing average processing latency.
    auto num_latencies = processing_latencies_ms_.size();
    if (num_latencies == SLIDING_WINDOW_SIZE) {
      // Pop the oldest latency from the queue and remove it from the running
      // sum.
      double oldest_latency_ms = processing_latencies_ms_.front();
      processing_latencies_ms_.pop();
      processing_latencies_sum_ms_ -= oldest_latency_ms;
    }
    // Add the new latency to the queue and the running sum.
    processing_latencies_ms_.push(processing_latency_ms);
    processing_latencies_sum_ms_ += processing_latency_ms;
    trailing_avg_processing_latency_ms_ =
        processing_latencies_sum_ms_ / num_latencies;
  }
}

bool Operator::IsStarted() const { return !stopped_; }

double Operator::GetTrailingAvgProcessingLatencyMs() const {
  return trailing_avg_processing_latency_ms_;
}

double Operator::GetHistoricalProcessFps() {
  return num_frames_processed_ / (op_timer_.ElapsedMSec() / 1000);
}

double Operator::GetAvgProcessingLatencyMs() const {
  return avg_processing_latency_ms_;
}

double Operator::GetAvgQueueLatencyMs() const {
  return queue_latency_sum_ms_ / num_frames_processed_;
}

OperatorType Operator::GetType() const { return type_; }

std::string Operator::GetName() const {
  return GetStringForOperatorType(GetType());
}

zmq::socket_t* Operator::GetControlSocket() { return control_socket_; };

void Operator::SetBlockOnPush(bool block) { block_on_push_ = block; }

void Operator::PushFrame(const std::string& sink_name,
                         std::unique_ptr<Frame> frame) {
  CHECK(sinks_.count(sink_name) != 0)
      << GetStringForOperatorType(GetType()) << " does not have a sink named \""
      << sink_name << "\"!";
  if (!processing_start_micros_.is_not_a_date_time()) {
    frame->SetValue(GetName() + ".total_micros",
                    boost::posix_time::microsec_clock::local_time() -
                        processing_start_micros_);
  }
  if (frame->IsStopFrame()) {
    found_last_frame_ = true;
  }
  sinks_[sink_name]->PushFrame(std::move(frame), block_on_push_);
}

std::unique_ptr<Frame> Operator::GetFrame(const std::string& source_name) {
  if (sources_.find(source_name) == sources_.end()) {
    std::ostringstream msg;
    msg << "\"" << source_name << "\" is not a valid source for operator \""
        << GetStringForOperatorType(GetType()) << "\".";
    throw std::out_of_range(msg.str());
  }
  if (source_frame_cache_.find(source_name) == source_frame_cache_.end()) {
    // This is for the nonblocking feature. Sources of some operators (those
    // with multiple sources) may not receive a frame within a PopFrame
    // duration. In this case, we return a nullptr. The caller should handle
    // this nullptr case.
    return nullptr;
  }
  return std::move(source_frame_cache_[source_name]);
}

std::unique_ptr<Frame> Operator::GetFrameDirect(
    const std::string& source_name) {
  if (readers_.find(source_name) == readers_.end()) {
    std::ostringstream msg;
    msg << "\""
        << "\" is not a valid source for operator \""
        << GetStringForOperatorType(GetType()) << "\".";
    throw std::out_of_range(msg.str());
  }
  return readers_.at(source_name)->PopFrame();
}
