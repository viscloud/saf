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

#include "litesql_writer.h"

#include <sstream>

#include <boost/date_time/posix_time/posix_time.hpp>

constexpr auto SOURCE_NAME = "input";

LiteSqlWriter::LiteSqlWriter(const std::string& output_dir)
    : Operator(OPERATOR_TYPE_CUSTOM, {SOURCE_NAME}, {}),
      output_dir_(output_dir) {
  while (output_dir_.back() == '/') {
    output_dir_.pop_back();
  }
  // Create the output directory if it doesn't exist
  if (!CreateDirs(output_dir_)) {
    LOG(INFO) << "Using existing directory: \"" << output_dir_ << "\"";
  }
}

std::shared_ptr<LiteSqlWriter> LiteSqlWriter::Create(
    const FactoryParamsType& params) {
  return std::make_shared<LiteSqlWriter>(params.at("output_dir"));
}

void LiteSqlWriter::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

bool LiteSqlWriter::Init() { return true; }

bool LiteSqlWriter::OnStop() { return true; }

void LiteSqlWriter::Process() {
  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);

  auto capture_time_micros =
      frame->GetValue<boost::posix_time::ptime>(Camera::kCaptureTimeMicrosKey);
  std::string data_dir =
      GetAndCreateDateTimeDir(output_dir_, capture_time_micros);

  std::string jpeg_path;
  if (frame->Count(JpegWriter::kPathKey)) {
    jpeg_path = frame->GetValue<std::string>(JpegWriter::kPathKey);
  }

  std::string compression_type;
  if (frame->Count(Compressor::kTypeKey)) {
    compression_type = frame->GetValue<std::string>(Compressor::kTypeKey);
  } else {
    compression_type =
        Compressor::CompressionTypeToString(Compressor::CompressionType::NONE);
  }

  std::ostringstream db_path;
  db_path << output_dir_ + "/frames.db";
  std::string db_path_str = db_path.str();
  FramesDB db("sqlite3", "database=" + db_path_str);
  std::string db_type;
  try {
    db.create();
    db_type = "new";
  } catch (litesql::Except e) {
    LOG(ERROR) << e.what();
    db_type = "existing";
  }
  LOG(INFO) << "Using " << db_type << " database: \"" << db_path_str << "\"";

  try {
    db.begin();
    db.verbose = true;

    FrameEntry fe(db);
    fe.dir = data_dir;
    fe.captureTimeMicros = GetDateTimeString(capture_time_micros);
    fe.jpegPath = jpeg_path;
    fe.compressionType = compression_type;
    fe.exposure = frame->GetValue<float>("CameraSettings.Exposure");
    fe.sharpness = frame->GetValue<float>("CameraSettings.Sharpness");
    fe.brightness = frame->GetValue<float>("CameraSettings.Brightness");
    fe.saturation = frame->GetValue<float>("CameraSettings.Saturation");
    fe.hue = frame->GetValue<float>("CameraSettings.Hue");
    fe.gain = frame->GetValue<float>("CameraSettings.Gain");
    fe.gamma = frame->GetValue<float>("CameraSettings.Gamma");
    fe.wbred = frame->GetValue<float>("CameraSettings.WBRed");
    fe.wbblue = frame->GetValue<float>("CameraSettings.WBBlue");
    fe.update();

    db.commit();
  } catch (litesql::Except e) {
    LOG(FATAL) << "\"" << db_path_str << "\""
               << " does not appear to be a valid sqlite3 database!"
               << std::endl
               << e.what();
  }
}
