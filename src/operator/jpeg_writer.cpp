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

#include "operator/jpeg_writer.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/variant/get.hpp>
#include <opencv2/opencv.hpp>

#include "camera/camera.h"
#include "stream/frame.h"
#include "utils/time_utils.h"

constexpr auto SOURCE_NAME = "input";
constexpr auto SINK_NAME = "output";

const char* JpegWriter::kPathKey = "JpegWriter.path";
const char* JpegWriter::kFieldKey = "JpegWriter.field";

JpegWriter::JpegWriter(const std::string& field, const std::string& output_dir,
                       bool organize_by_time, unsigned long frames_per_dir)
    : Operator(OPERATOR_TYPE_JPEG_WRITER, {SOURCE_NAME}, {SINK_NAME}),
      field_(field),
      tracker_{output_dir, organize_by_time, frames_per_dir} {}

std::shared_ptr<JpegWriter> JpegWriter::Create(
    const FactoryParamsType& params) {
  std::string field = params.at("field");
  std::string output_dir = params.at("output_dir");
  bool organize_by_time = params.at("organize_by_time") == "1";
  unsigned long frames_per_dir = std::stoul(params.at("frames_per_dir"));
  return std::make_shared<JpegWriter>(field, output_dir, organize_by_time,
                                      frames_per_dir);
}

void JpegWriter::SetSource(StreamPtr stream) {
  Operator::SetSource(SOURCE_NAME, stream);
}

StreamPtr JpegWriter::GetSink() { return Operator::GetSink(SINK_NAME); }

bool JpegWriter::Init() { return true; }

bool JpegWriter::OnStop() { return true; }

void JpegWriter::Process() {
  std::unique_ptr<Frame> frame = GetFrame(SOURCE_NAME);
  cv::Mat img = frame->GetValue<cv::Mat>(field_);

  auto capture_time_micros =
      frame->GetValue<boost::posix_time::ptime>(Camera::kCaptureTimeMicrosKey);
  std::ostringstream filepath;
  filepath << tracker_.GetAndCreateOutputDir(capture_time_micros)
           << GetDateTimeString(capture_time_micros) << "_" << field_ << ".jpg";
  std::string filepath_s = filepath.str();
  try {
    cv::imwrite(filepath_s, img);
  } catch (cv::Exception& e) {
    LOG(FATAL) << "Unable to write JPEG file \"" << filepath_s
               << "\": " << e.what();
  }

  frame->SetValue(kPathKey, filepath_s);
  frame->SetValue(kFieldKey, field_);
  PushFrame(SINK_NAME, std::move(frame));
};
