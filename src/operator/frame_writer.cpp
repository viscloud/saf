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

#include "operator/frame_writer.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/archive/archive_exception.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "camera/camera.h"
#include "stream/frame.h"
#include "utils/time_utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

FrameWriter::FrameWriter(const std::unordered_set<std::string> fields,
                         const std::string& output_dir, const FileFormat format,
                         bool save_fields_separately, bool organize_by_time,
                         unsigned long frames_per_dir)
    : Operator(OPERATOR_TYPE_FRAME_WRITER, {SOURCE_NAME}, {SINK_NAME}),
      fields_(fields),
      format_(format),
      save_fields_separately_(save_fields_separately),
      tracker_{output_dir, organize_by_time, frames_per_dir} {}

std::shared_ptr<FrameWriter> FrameWriter::Create(
    const FactoryParamsType& params) {
  // TODO: Parse field names. Currently, FactoryParamsType does not support keys
  //       that are sets.
  std::unordered_set<std::string> fields;

  std::string output_dir = params.at("output_dir");

  std::string format_s = params.at("format");
  FileFormat format;
  if (format_s == "binary") {
    format = BINARY;
  } else if (format_s == "json") {
    format = JSON;
  } else if (format_s == "text") {
    format = TEXT;
  } else {
    LOG(FATAL) << "Unknown file format: " << format_s;
  }

  bool save_fields_separately = params.at("save_field_separately") == "1";
  bool organize_by_time = params.at("organize_by_time") == "1";
  unsigned long frames_per_dir = std::stoul(params.at("frames_per_dir"));
  return std::make_shared<FrameWriter>(fields, output_dir, format,
                                       save_fields_separately, organize_by_time,
                                       frames_per_dir);
}

void FrameWriter::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr FrameWriter::GetSink() { return Operator::GetSink(SINK_NAME); }

bool FrameWriter::Init() { return true; }

bool FrameWriter::OnStop() { return true; }

void FrameWriter::Process() {
  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);
  auto frame_to_write = std::make_unique<Frame>(frame, fields_);

  auto capture_time_micros =
      frame->GetValue<boost::posix_time::ptime>(Camera::kCaptureTimeMicrosKey);
  std::ostringstream base_filepath;
  base_filepath << tracker_.GetAndCreateOutputDir(capture_time_micros)
                << GetDateTimeString(capture_time_micros);

  if (save_fields_separately_) {
    // Create a separate file for each field.
    for (const auto& p : frame_to_write->GetFields()) {
      std::string key = p.first;
      Frame::field_types value = p.second;

      // The final filepath needs the key and an extension.
      std::ostringstream filepath;
      filepath << base_filepath.str() << "_" << key << GetExtension();

      std::string filepath_s = filepath.str();
      std::ofstream file(filepath_s, std::ios::binary | std::ios::out);
      if (!file.is_open()) {
        LOG(FATAL) << "Unable to open file \"" << filepath_s << "\".";
      }

      try {
        switch (format_) {
          case BINARY: {
            boost::archive::binary_oarchive ar(file);
            ar << value;
            break;
          }
          case JSON: {
            file << frame_to_write->GetFieldJson(key).dump(4);
            break;
          }
          case TEXT: {
            boost::archive::text_oarchive ar(file);
            ar << value;
            break;
          }
        }
      } catch (const boost::archive::archive_exception& e) {
        LOG(FATAL) << "Boost serialization error: " << e.what();
      }

      file.close();
      if (!file) {
        LOG(FATAL) << "Unknown error while writing binary file \"" << filepath_s
                   << "\".";
      }
    }
  } else {
    // The final filepath just needs an extension.
    base_filepath << GetExtension();

    std::string filepath_s = base_filepath.str();
    std::ofstream file(filepath_s, std::ios::binary | std::ios::out);
    if (!file.is_open()) {
      LOG(FATAL) << "Unable to open file \"" << filepath_s << "\".";
    }

    try {
      switch (format_) {
        case BINARY: {
          boost::archive::binary_oarchive ar(file);
          ar << frame_to_write;
          break;
        }
        case JSON: {
          file << frame_to_write->ToJson().dump(4);
          break;
        }
        case TEXT: {
          boost::archive::text_oarchive ar(file);
          ar << frame_to_write;
          break;
        }
      }
    } catch (const boost::archive::archive_exception& e) {
      LOG(FATAL) << "Boost serialization error: " << e.what();
    }

    file.close();
    if (!file) {
      LOG(FATAL) << "Unknown error while writing binary file \"" << filepath_s
                 << "\".";
    }
  }

  PushFrame(SINK_NAME, std::move(frame));
}

std::string FrameWriter::GetExtension() {
  switch (format_) {
    case BINARY:
      return ".bin";
    case JSON:
      return ".json";
    case TEXT:
      return ".txt";
  }

  LOG(FATAL) << "Unhandled FileFormat: " << format_;
}
