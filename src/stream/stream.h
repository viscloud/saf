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

#ifndef SAF_STREAM_STREAM_H_
#define SAF_STREAM_STREAM_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <unordered_set>

#include "common/timer.h"
#include "frame.h"

/**
 * @brief A reader that reads from a stream. There could be multiple readers
 * reading from the same stream.
 */
class StreamReader {
  friend class Stream;

 public:
  StreamReader(Stream* stream, size_t max_buffer_size = 16);

  /**
   * @brief Pop a frame, and timeout if no frame available for a given time
   * @param timeout_ms Time out threshold, 0 for forever
   * WARNING: caller is expected to check for nullptr
   */
  std::unique_ptr<Frame> PopFrame(unsigned int timeout_ms = 0);

  void UnSubscribe();
  double GetPushFps();
  double GetPopFps();
  double GetHistoricalFps();
  // Signals that this StreamReader should stop any currently-waiting attempts
  // to push or pop frames. This is required because Operator::Stop() joins the
  // processing threads, and the processing threads may call
  // StreamReader::PushFrame() and StreamReader::PopFrame() in a blocking
  // manner.
  void Stop();

 private:
  /**
   * @brief Push a frame into the stream.
   * @param frame The frame to be pushed into the stream.
   */
  void PushFrame(std::unique_ptr<Frame> frame, bool block = false);
  Stream* stream_;
  // Max size of the buffer to hold frames in the stream
  size_t max_buffer_size_;
  // The frame buffer
  std::queue<std::unique_ptr<Frame>> frame_buffer_;
  std::mutex mtx_;
  // Used to wait if the queue is full when trying to push.
  std::condition_variable push_cv_;
  // Used to wait if the queue is empty when trying to pop.
  std::condition_variable pop_cv_;
  // Used to signal PushFrame() and PopFrame() that they should return
  // immediately.
  std::atomic<bool> stopped_;

  // The total number of frames that have popped from this StreamReader.
  unsigned long num_frames_popped_;
  // Milliseconds between when this StreamReader was constructed and when the
  // first frame was popped. -1 means that this has not been set yet.
  double first_frame_pop_ms_;
  // Alpha parameter for the exponentially weighted moving average (EWMA)
  // formula.
  double alpha_;
  // The EWMA of the milliseconds between frame pushes.
  double running_push_ms_;
  // The EWMA of the milliseconds between frame pops.
  double running_pop_ms_;
  // Milliseconds between when this StreamReader was constructed and the last
  // frame push.
  double last_push_ms_;
  // Milliseconds between when this StreamReader was constructed and the last
  // frame pop.
  double last_pop_ms_;
  // Started when this StreamReader is constructed.
  Timer timer_;
};

/**
 * @brief A stream is a serious of data, the data itself could be stats, images,
 * or simply a bunch of bytes.
 */
class Stream {
 public:
  Stream(std::string name = "");
  /**
   * @brief Push a frame into the stream.
   * @param frame The frame to be pushed into the stream.
   * @param block Whether to block if any of the StreamReaders are full.
   */
  void PushFrame(std::unique_ptr<Frame> frame, bool block = false);

  /**
   * @brief Get the name of the stream.
   */
  std::string GetName() { return name_; }

  /**
   * @brief Get a reader from the stream.
   * @param max_buffer_size The buffer size limit of the reader.
   */
  StreamReader* Subscribe(size_t max_buffer_size = 16);

  /**
   * @brief Unsubscribe from the stream
   */
  void UnSubscribe(StreamReader* reader);

  // Stops all of the StreamReaders attached to this Stream, waking up any
  // threads that are trying to push or pop frames from this Stream. See the
  // documentation for StreamReader::Stop().
  void Stop();

 private:
  // Stream name for profiling and debugging
  std::string name_;
  // The readers of the stream
  std::vector<std::shared_ptr<StreamReader>> readers_;
  // Stream lock
  std::mutex stream_lock_;
};

#endif  // SAF_STREAM_STREAM_H_
