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
//   DO NOT compile with -ffast-math.

#include "ncs_manager.h"

extern "C" {
#include "mvnc.h"
#include "utils/fp16.h"
}

#include <random>

#define NCS_NAME_SIZE (28)

typedef unsigned short half;

inline static half* load_image(const cv::Mat& image, const cv::Size& size,
                               const float* mean, const float* std) {
  float* data32;
  half* data16;
  cv::Mat image32;

  cv::resize(image, image32, size);
  cv::cvtColor(image32, image32, CV_BGR2RGB);
  image32.convertTo(image32, CV_32FC3, 1. / 255.);
  data32 = (float*)image32.data;
  data16 = (half*)malloc(sizeof(half) * image32.total() * image32.channels());
  if (!data16) {
    throw std::runtime_error("Failed to allocate half precision array");
  }
  for (decltype(image32.total()) i = 0; i < image32.total(); i++) {
    data32[3 * i] = (data32[3 * i] - mean[0]) * std[0];
    data32[3 * i + 1] = (data32[3 * i + 1] - mean[1]) * std[1];
    data32[3 * i + 2] = (data32[3 * i + 2] - mean[2]) * std[2];
  }
  floattofp16((unsigned char*)data16, data32,
              image32.total() * image32.channels());

  return data16;
}

void* NCSManager::LoadGraph(const char* path, unsigned int* length) {
  FILE* fp;
  char* buf;

  fp = fopen(path, "rb");
  if (!fp) return nullptr;
  fseek(fp, 0, SEEK_END);
  *length = ftell(fp);
  rewind(fp);
  if (!(buf = (char*)malloc(*length))) {
    fclose(fp);
    return 0;
  }
  if (fread(buf, 1, *length, fp) != *length) {
    fclose(fp);
    free(buf);
    return 0;
  }
  fclose(fp);

  return buf;
}

NCSManager::NCSManager(const char* model_path, int dim)
    : _model_path(model_path),
      _image_size(dim, dim),
      _input_size(3 * dim * dim * sizeof(half)),
      _mean({0.0f, 0.0f, 0.0f}),
      _std({1.0f, 1.0f, 1.0f}),
      _iq(10240) {}

int NCSManager::Open() {
  EnumerateDevices(_names);

  _devices.reserve(_names.size());
  _graphs.reserve(_names.size());

  for (decltype(_names.size()) i = 0; i < _names.size(); i++) {
    _devices.push_back(OpenDevice(_names[i]));
    _graphs.push_back(AllocateGraph(_devices[i], _model_path.c_str()));
  }

  Start();

  return _names.size();
}

void NCSManager::Start() {
  _done = false;
  _it = new std::thread([&] {
    std::pair<int, void*> p;
    while (!_done) {
      while (_iq.pop(p)) {
        int i = p.first;
        half* tensor = (half*)p.second;
        if (mvncLoadTensor(_graphs[i], tensor, _input_size, 0 /* userobj */)) {
          free(tensor);
          throw std::runtime_error("Failed to load tensor");
        }
        free(tensor);
      }
      std::this_thread::yield();
    }
  });
}

void NCSManager::LoadImage(const cv::Mat& image) {
  assert(_names.size() > 0);
  LoadImage(0, image);  // TODO
}

void NCSManager::LoadImage(const char* filename) {
  LoadImage(cv::imread(filename));
}

void NCSManager::GetResult(std::vector<float>& result) {
  assert(_names.size() > 0);
  GetResult(0, result);  // TODO
}

void NCSManager::LoadImageAndGetResult(std::vector<float>& result,
                                       const char* filename) {
  LoadImageAndGetResult(result, cv::imread(filename));
}

void NCSManager::LoadImageAndGetResult(std::vector<float>& result,
                                       const cv::Mat& image) {
  static std::random_device rd;
  static std::mt19937 gen(rd());

  int i = std::uniform_int_distribution<>{0, ((int)_names.size()) - 1}(gen);

  LoadImage(i, image);
  GetResult(i, result);
}

void NCSManager::Stop() {
  _done = true;
  _it->join();
  _it = nullptr;
}

void NCSManager::Close() {
  Stop();
  for (decltype(_devices.size()) i = 0; i < _devices.size(); i++) {
    DeallocateGraph(_graphs[i]);
    CloseDevice(_devices[i]);
  }
  _names.clear();
  _devices.clear();
  _graphs.clear();
}

NCSManager::~NCSManager() {}

int NCSManager::EnumerateDevices(std::vector<std::string>& names) {
  char buffer[NCS_NAME_SIZE];
  names.clear();
  for (int i = 0; i < 16; i++) {
    if (mvncGetDeviceName(i, buffer, NCS_NAME_SIZE) == 0) {
      names.push_back(std::string(buffer));
    } else {
      break;
    }
  }
  return names.size();
}

void NCSManager::LoadImage(int i, const char* filename) {
  LoadImage(i, cv::imread(filename));
}

void NCSManager::LoadImage(int i, const cv::Mat& image) {
  half* tensor = load_image(image, _image_size, _mean, _std);
  while (!_iq.push(std::make_pair(i, tensor)))
    ;
}

void NCSManager::GetResult(int i, std::vector<float>& result) {
  void* result16;
  float* result32;
  unsigned int len;
  void* userobj;
  if (mvncGetResult(_graphs[i], &result16, &len, &userobj)) {
    throw std::runtime_error("Failed to get result");
  }
  len /= sizeof(half);
  result32 = (float*)malloc(len * sizeof(float));
  fp16tofloat(result32, (unsigned char*)result16, len);

  result.clear();
  result.assign(result32, result32 + len);

  free(result32);
}

bool NCSManager::IsOpened() const { return GetNumDevices() > 0; }

int NCSManager::GetNumDevices() const { return _names.size(); }

void* NCSManager::OpenDevice(const std::string& name) {
  void* handle = nullptr;
  if (mvncOpenDevice(name.c_str(), &handle)) {
    throw std::runtime_error("Failed to open device");
  }
  return handle;
}

void* NCSManager::AllocateGraph(void* handle, const char* path) {
  unsigned int len = 0;
  void* graphHandle;
  void* graphFile = LoadGraph(path, &len);
  if (mvncAllocateGraph(handle, &graphHandle, graphFile, len)) {
    throw std::runtime_error("Failed to allocate graph");
  }
  return graphHandle;
}

void NCSManager::DeallocateGraph(void* handle) {
  if (mvncDeallocateGraph(handle)) {
    throw std::runtime_error("Failed to deallocate graph");
  }
}

void NCSManager::CloseDevice(void* handle) {
  if (mvncCloseDevice(handle)) {
    throw std::runtime_error("Failed to close device");
  }
}
