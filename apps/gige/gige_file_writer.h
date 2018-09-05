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

#ifndef SAF_GIGE_GIGE_FILE_WRITER_H_
#define SAF_GIGE_GIGE_FILE_WRITER_H_

#include <stdlib.h>
#include <fstream>

#include "saf.h"

/**
 * @brief A file writer that writes raw bytes to a file.
 */
class GigeFileWriter : public Operator {
 public:
  /**
   * @brief FileWriter constructor
   * @param directory Directroy to store the file.
   * @param filename_base The basename of a file, file will be named as
   * {frame_count / frames_per_file}.dat
   * @param frames_per_file Number of frames per file.
   */
  GigeFileWriter(const std::string& directory, size_t frames_per_file);

  size_t GetFramesWritten() { return frames_written_; }
  std::string GetCurrentFilename() { return current_filename_; }
  std::string GetCurrentDirectory() { return directory_; }
  void SetDirectory(const std::string& directory) { directory_ = directory; }

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::string directory_;
  std::string current_filename_;
  size_t frames_written_;
  size_t frames_per_file_;

  std::ofstream current_file_;
};

#endif  // SAF_GIGE_GIGE_FILE_WRITER_H_
