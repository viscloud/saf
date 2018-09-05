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

#ifndef SAF_OPERATOR_OPERATOR_H_
#define SAF_OPERATOR_OPERATOR_H_

#include <atomic>
#include <queue>
#include <thread>
#include <unordered_map>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <zmq.hpp>

#include "stream/stream.h"

class Pipeline;

#ifdef USE_VIMBA
class VimbaCameraFrameObserver;
#endif  // USE_VIMBA

/**
 * @brief Operator is the core computation unit in the system. It accepts
 * frames from one or more source streams, and output frames to one or more sink
 * streams.
 */
class Operator {
  friend class Pipeline;
#ifdef USE_VIMBA
  friend class VimbaCameraFrameObserver;
#endif  // USE_VIMBA
 public:
  Operator(OperatorType type, const std::vector<std::string>& source_names,
           const std::vector<std::string>& sink_names);
  virtual ~Operator();
  /**
   * @brief Start processing, drain frames from sources and send output to
   * sinks.
   * @return True if start successfully, false otherwise.
   */
  bool Start(size_t buf_size = 16);
  /**
   * @brief Stop processing
   * @return True if stop sucsessfully, false otherwise.
   */
  bool Stop();

  /**
   * @brief Override a sink stream.
   * @param name Name of the sink.
   * @param stream Stream to be set.
   */
  void SetSink(const std::string& name, StreamPtr stream);

  /**
   * @brief Get sink stream of the operator by name.
   * @param name Name of the sink.
   * @return Stream with the name.
   */
  StreamPtr GetSink(const std::string& name);

  /**
   * @brief Set the source of the operator by name.
   * @param name Name of the source.
   * @param stream Stream to be set.
   */
  virtual void SetSource(const std::string& name, StreamPtr stream);

  /**
   * @brief Check if the operator has started.
   * @return True if operator has started.
   */
  bool IsStarted() const;

  /**
   * @brief Get trailing (sliding window) average latency of the operator.
   * @return Latency in ms.
   */
  virtual double GetTrailingAvgProcessingLatencyMs() const;

  /**
   * @brief Get overall average latency.
   * @return Latency in ms.
   */
  virtual double GetAvgProcessingLatencyMs() const;

  /**
   * @brief Get overall average queue latency.
   * @return queue latency in ms.
   */
  virtual double GetAvgQueueLatencyMs() const;

  /**
   * @brief Get overall throughput of operator.
   * @return in FPS.
   */
  virtual double GetHistoricalProcessFps();

  /**
   * @brief Get the type of the operator
   */
  OperatorType GetType() const;

  /**
   * @brief Get the name of the operator
   */
  virtual std::string GetName() const;

  zmq::socket_t* GetControlSocket();

  // Configure whether this operator should block when pushing frames to its
  // outputs streams if any of its output streams is full.
  virtual void SetBlockOnPush(bool block);

 protected:
  /**
   * @brief Initialize the operator.
   */
  virtual bool Init() = 0;
  /**
   * @brief Called after Operator#Stop() is called, do any clean up in this
   * method.
   * @return [description]
   */
  virtual bool OnStop() = 0;
  /**
   * @brief Fetch one frame from sources_ and process them.
   *
   * @input_frames A list of input frames feteched from sources_.
   * @return A list of output frames.
   */
  virtual void Process() = 0;

  std::unique_ptr<Frame> GetFrame(const std::string& source_name);
  std::unique_ptr<Frame> GetFrameDirect(const std::string& source_name);
  void PushFrame(const std::string& sink_name, std::unique_ptr<Frame> frame);
  void OperatorLoop();
  void OperatorLoopDirect();

  std::unordered_map<std::string, std::unique_ptr<Frame>> source_frame_cache_;
  std::unordered_map<std::string, StreamPtr> sources_;
  std::unordered_map<std::string, StreamPtr> sinks_;
  std::unordered_map<std::string, StreamReader*> readers_;

  std::thread process_thread_;
  std::atomic<bool> stopped_;
  std::atomic<bool> found_last_frame_;

  unsigned int num_frames_processed_;
  double avg_processing_latency_ms_;
  // A queue of recent processing latencies.
  std::queue<double> processing_latencies_ms_;
  // The sum of all of the values in "processing_latencies_ms_". This is purely
  // a performance optimization to avoid needing to sum over
  // "processing_latencies_ms_" for every frame.
  double processing_latencies_sum_ms_;
  // Processing latency, computed using a sliding window average.
  double trailing_avg_processing_latency_ms_;
  double queue_latency_sum_ms_;

 private:
  const OperatorType type_;
  zmq::socket_t* control_socket_;
  Timer op_timer_;
  // Whether to block when pushing frames if any output streams are full.
  std::atomic<bool> block_on_push_;
  boost::posix_time::ptime processing_start_micros_;
};

#endif  // SAF_OPERATOR_OPERATOR_H_
