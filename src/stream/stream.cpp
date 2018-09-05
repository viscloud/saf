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

#include "stream/stream.h"

#include <chrono>

#include "operator/flow_control/flow_control_entrance.h"

/////// Stream

Stream::Stream(std::string name) : name_(name) {}

constexpr unsigned int ms_per_sec = 1000;

StreamReader* Stream::Subscribe(size_t max_buffer_size) {
  std::lock_guard<std::mutex> guard(stream_lock_);
  auto reader = std::make_shared<StreamReader>(this, max_buffer_size);
  readers_.push_back(reader);
  return reader.get();
}

void Stream::UnSubscribe(StreamReader* reader) {
  std::lock_guard<std::mutex> guard(stream_lock_);
  readers_.erase(std::remove_if(readers_.begin(), readers_.end(),
                                [reader](std::shared_ptr<StreamReader> sr) {
                                  return sr.get() == reader;
                                }));
}

void Stream::PushFrame(std::unique_ptr<Frame> frame, bool block) {
  // Make a copy of readers_ so that we can block without holding stream_lock_.
  std::vector<std::shared_ptr<StreamReader>> readers;
  {
    std::lock_guard<std::mutex> guard(stream_lock_);
    readers = readers_;
  }

  decltype(readers.size()) num_readers = readers.size();
  if (num_readers == 0) {
    VLOG(1) << "No readers. Dropping frame: "
            << frame->GetValue<unsigned long>("frame_id");
  } else if (num_readers == 1) {
    readers.at(0)->PushFrame(std::move(frame), block);
  } else {
    // If there is more than one reader, then we need to copy the frame.
    for (const auto& reader : readers) {
      reader->PushFrame(std::make_unique<Frame>(frame), block);
    }
  }
}

void Stream::Stop() {
  std::lock_guard<std::mutex> lock(stream_lock_);
  for (auto reader : readers_) {
    reader->Stop();
  }
}

/////// StreamReader
StreamReader::StreamReader(Stream* stream, size_t max_buffer_size)
    : stream_(stream),
      max_buffer_size_(max_buffer_size),
      num_frames_popped_(0),
      first_frame_pop_ms_(-1),
      alpha_(0.25),
      running_push_ms_(0),
      running_pop_ms_(0),
      last_push_ms_(0),
      last_pop_ms_(0) {
  stopped_ = false;
  timer_.Start();
}

std::unique_ptr<Frame> StreamReader::PopFrame(unsigned int timeout_ms) {
  bool have_timeout = timeout_ms > 0;
  auto pred_func = [this] { return stopped_ || (frame_buffer_.size() > 0); };

  std::unique_lock<std::mutex> lock(mtx_);
  while (true) {
    bool pred;
    if (have_timeout) {
      // Wait for the specified time.
      pred = pop_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                              pred_func);
    } else {
      // Wait forever.
      pop_cv_.wait(lock, pred_func);
      pred = true;
    }

    if (stopped_) {
      // We stopped, so return early.
      return nullptr;
    } else if (pred) {
      // We either timed out or were notified. Either way, there is a frame
      // available so we break out of the loop and pop it.
      break;
    } else if (have_timeout) {
      // We must have woken up because of a timeout, so we return immediately.
      return nullptr;
    }
  }

  // Clear to pop frame.
  auto frame = std::move(frame_buffer_.front());
  frame_buffer_.pop();
  ++num_frames_popped_;

  double current_ms = timer_.ElapsedMSec();
  double delta_ms = current_ms - last_pop_ms_;
  running_pop_ms_ = running_pop_ms_ * (1 - alpha_) + delta_ms * alpha_;
  last_pop_ms_ = current_ms;

  if (first_frame_pop_ms_ == -1) {
    first_frame_pop_ms_ = current_ms;
  }

  // We freed a space in the queue, so notify anyone waiting to push.
  push_cv_.notify_one();

  return frame;
}

void StreamReader::PushFrame(std::unique_ptr<Frame> frame, bool block) {
  std::unique_lock<std::mutex> lock(mtx_);

  if (block) {
    push_cv_.wait(lock, [this] {
      return stopped_ || (frame_buffer_.size() < max_buffer_size_);
    });
    if (stopped_) {
      // We stopped, so return early.
      return;
    }
  } else if (frame_buffer_.size() >= max_buffer_size_) {
    // There is not enough space in the queue, and we're not supposed to block,
    // so we have no choice but to drop the frame.
    unsigned long id = frame->GetValue<unsigned long>("frame_id");
    LOG(WARNING) << "Stream queue full. Dropping frame: " << id;
    if (frame->GetFlowControlEntrance()) {
      // This scenario should not happen. If we're using end-to-end flow
      // control, then we should not be using so many tokens such that we are
      // dropping frames.
      LOG(ERROR) << "Dropped frame " << id << " while using end-to-end flow "
                 << "control. This should not have happened. Either increase "
                 << "the size of this stream's queue or decrease the number of "
                 << "flow control tokens.";
    }
    return;
  }

  // Clear to push frame.
  frame_buffer_.push(std::move(frame));

  double current_ms = timer_.ElapsedMSec();
  double delta_ms = current_ms - last_push_ms_;
  running_push_ms_ = running_push_ms_ * (1 - alpha_) + delta_ms * alpha_;
  last_push_ms_ = current_ms;

  // We pushed a frame, so notify any threads that are waiting to receive
  // frames.
  pop_cv_.notify_one();
}

void StreamReader::UnSubscribe() {
  Stop();
  stream_->UnSubscribe(this);
}

double StreamReader::GetPushFps() { return ms_per_sec / running_push_ms_; }

double StreamReader::GetPopFps() { return ms_per_sec / running_pop_ms_; }

double StreamReader::GetHistoricalFps() {
  return num_frames_popped_ /
         ((timer_.ElapsedMSec() - first_frame_pop_ms_) / ms_per_sec);
}

void StreamReader::Stop() {
  stopped_ = true;
  std::unique_lock<std::mutex> lock(mtx_);
  // Wake up any threads that are waiting to push or pop frames.
  push_cv_.notify_all();
  pop_cv_.notify_all();
}
