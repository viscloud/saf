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

// An interface to Neural Compute Stick

#ifndef SAF_NCS_NCS_MANAGER_H_
#define SAF_NCS_NCS_MANAGER_H_

#include <string>
#include <vector>

#include "opencv2/opencv.hpp"

#include <atomic>
#include <thread>
#include "boost/lockfree/spsc_queue.hpp"

class NCSManager {
 private:
  const std::string _model_path;
  const cv::Size _image_size;
  const unsigned int _input_size;
  const float _mean[3];
  const float _std[3];

  std::atomic<bool> _done;
  std::thread* _it;
  boost::lockfree::spsc_queue<std::pair<int, void*>,
                              boost::lockfree::fixed_sized<false>>
      _iq;

  // TODO: SPSC queue for output as well?

  std::vector<std::string> _names;
  std::vector<void*> _devices;
  std::vector<void*> _graphs;

  void Start();
  void Stop();

  static void* LoadGraph(const char* path, unsigned int* length);

 public:
  NCSManager(const char* model_path, int dim);

  int Open();

  void LoadImage(const char* filename);
  void LoadImage(const cv::Mat& image);
  void GetResult(std::vector<float>& result);

  void LoadImage(int i, const char* filename);
  void LoadImage(int i, const cv::Mat& image);
  void GetResult(int i, std::vector<float>& result);

  void LoadImageAndGetResult(std::vector<float>& result, const char* filename);
  void LoadImageAndGetResult(std::vector<float>& result, const cv::Mat& image);

  void Close();

  bool IsOpened() const;
  int GetNumDevices() const;

  static int EnumerateDevices(std::vector<std::string>& names);

  static void* OpenDevice(const std::string& name);
  static void CloseDevice(void* handle);

  static void* AllocateGraph(void* handle, const char* path);
  static void DeallocateGraph(void* handle);

  ~NCSManager();
};

#endif  // SAF_NCS_NCS_MANAGER_H_
