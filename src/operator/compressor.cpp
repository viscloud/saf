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

#include "operator/compressor.h"

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "utils/utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";
constexpr auto FIELD_TO_COMPRESS = "original_bytes";

const char* Compressor::kDataKey = "Compressor.compressed_bytes";
const char* Compressor::kTypeKey = "Compressor.compression_type";

Compressor::Compressor(CompressionType t)
    : Operator(OPERATOR_TYPE_COMPRESSOR, {SOURCE_NAME}, {SINK_NAME}),
      compression_type_(t),
      stop_(false) {
  output_thread_ = std::thread(&Compressor::OutputFrames, this);
}

Compressor::~Compressor() {
  stop_ = true;
  queue_cond_.notify_one();
  output_thread_.join();
}

std::shared_ptr<Compressor> Compressor::Create(const FactoryParamsType&) {
  SAF_NOT_IMPLEMENTED;
  return nullptr;
}

std::string Compressor::CompressionTypeToString(
    Compressor::CompressionType type) {
  switch (type) {
    case BZIP2:
      return "bzip2";
    case GZIP:
      return "gzip";
    case NONE:
      return "none";
  }

  LOG(FATAL) << "Unhandled CompressionType: " << type;
}

void Compressor::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr Compressor::GetSink() { return Operator::GetSink(SINK_NAME); }

bool Compressor::Init() { return true; }

bool Compressor::OnStop() { return true; }

void Compressor::Process() {
  auto frame = GetFrame(SOURCE_NAME);
  auto future = std::async(std::launch::async,
                           [this, frame = std::move(frame)]() mutable {
                             return this->CompressFrame(std::move(frame));
                           });
  {
    std::lock_guard<std::mutex> guard(queue_mutex_);
    queue_.push(std::move(future));
    queue_cond_.notify_one();
  }
}

void Compressor::OutputFrames() {
  while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cond_.wait(lock, [this]() { return stop_ || !queue_.empty(); });
    if (stop_) {
      break;
    }
    auto future = std::move(queue_.front());
    queue_.pop();
    auto compressed_frame = future.get();
    PushFrame(SINK_NAME, std::move(compressed_frame));
  }
}

std::unique_ptr<Frame> Compressor::CompressFrame(std::unique_ptr<Frame> frame) {
  auto raw_image = frame->GetValue<std::vector<char>>(FIELD_TO_COMPRESS);

  std::vector<char> compressed_raw;
  boost::iostreams::filtering_ostream compressor;
  if (compression_type_ == CompressionType::BZIP2) {
    compressor.push(boost::iostreams::bzip2_compressor());
  } else if (compression_type_ == CompressionType::GZIP) {
    compressor.push(boost::iostreams::gzip_compressor());
  }
  compressor.push(boost::iostreams::back_inserter(compressed_raw));
  compressor.write((char*)raw_image.data(), raw_image.size());
  boost::iostreams::close(compressor);

  // Write compressed data to the frame and notify committer thread
  frame->SetValue(kDataKey, compressed_raw);
  frame->SetValue(kTypeKey, CompressionTypeToString(compression_type_));

  return frame;
}
