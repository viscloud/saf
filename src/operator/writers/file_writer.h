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

// Write metadata to filesystem

#ifndef SAF_OPERATOR_WRITERS_FILE_WRITER_H_
#define SAF_OPERATOR_WRITERS_FILE_WRITER_H_

#include <cstdlib>
#include <fstream>

#include "camera/camera.h"
#include "operator/writers/writer.h"

class FileWriter : public BaseWriter {
 public:
  FileWriter(const std::string& uri) : uri_(uri) {}

  virtual ~FileWriter() {
    if (ofs_.is_open()) ofs_.close();
  }

  virtual bool Init() {
    CHECK(!uri_.empty());
    ofs_.open(uri_);
    CHECK(ofs_.is_open()) << "Error opening file " << uri_;
    return true;
  }

  virtual void Write(const std::unique_ptr<Frame>& frame) {
    if (!frame) return;

    auto camera_name = frame->GetValue<std::string>("camera_name");
    auto timestamp = frame->GetValue<boost::posix_time::ptime>(
        Camera::kCaptureTimeMicrosKey);
    auto frame_id = frame->GetValue<unsigned long>("frame_id");
    auto tags = frame->GetValue<std::vector<std::string>>("tags");
    auto bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");

    if (bboxes.size() > 0) {
      CHECK(bboxes.size() == tags.size());
      if (frame->Count("ids") > 0) {
        auto ids = frame->GetValue<std::vector<std::string>>("ids");
        auto features =
            frame->GetValue<std::vector<std::vector<double>>>("features");
        CHECK(bboxes.size() == ids.size());
        CHECK(bboxes.size() == features.size());

        for (size_t i = 0; i < ids.size(); ++i) {
          ofs_ << camera_name << "," << ids[i] << "," << timestamp << ","
               << frame_id << "," << tags[i] << "," << bboxes[i].px << ";"
               << bboxes[i].py << ";" << bboxes[i].width << ";"
               << bboxes[i].height << ",";
          if (features[i].size() > 0) ofs_ << features[i][0];
          for (size_t j = 1; j < features[i].size(); j++) {
            ofs_ << ";" << features[i][j];
          }
          ofs_ << '\n';
        }
      }

      for (size_t i = 0; i < bboxes.size(); ++i) {
        ofs_ << camera_name << "," << timestamp << "," << frame_id << ","
             << tags[i] << "," << bboxes[i].px << ";" << bboxes[i].py << ";"
             << bboxes[i].width << ";" << bboxes[i].height << "\n";
      }
    }
  }

 private:
  std::string uri_;
  std::ofstream ofs_;
};

#endif  // SAF_OPERATOR_WRITERS_FILE_WRITER_H_
