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

#ifndef SAF_UTILS_CUDA_UTILS_H_
#define SAF_UTILS_CUDA_UTILS_H_

#include <glog/logging.h>
#include <string>

#ifdef USE_CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>

#define CUDA_CHECK(condition)                                         \
  /* Code block avoids redefinition of cudaError_t error */           \
  do {                                                                \
    cudaError_t error = condition;                                    \
    CHECK_EQ(error, cudaSuccess) << " " << cudaGetErrorString(error); \
  } while (0)

#endif  // USE_CUDA

/**
 * @brief Get a list of GPUs in machine.
 */
inline void GetCUDAGpus(std::vector<int>& gpus) {
  int count = 0;
#ifdef USE_CUDA
  CUDA_CHECK(cudaGetDeviceCount(&count));
#else
  LOG(FATAL) << "Can't use CUDA function in NO_GPU mode";
#endif  // USE_CUDA
  for (int i = 0; i < count; ++i) {
    gpus.push_back(i);
  }
}

#endif  // SAF_UTILS_CUDA_UTILS_H_
