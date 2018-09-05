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

#ifndef SAF_MODEL_MODEL_MANAGER_H_
#define SAF_MODEL_MODEL_MANAGER_H_

#include <unordered_map>

#include <opencv2/opencv.hpp>

#include "model.h"

/**
 * @brief A singleton class that controls all models.
 */
class ModelManager {
 public:
  static ModelManager& GetInstance();

 public:
  ModelManager();
  ModelManager(const ModelManager& other) = delete;
  cv::Scalar GetMeanColors() const;
  void SetMeanColors(cv::Scalar mean_colors);
  std::unordered_map<std::string, std::vector<ModelDesc>> GetAllModelDescs()
      const;
  ModelDesc GetModelDesc(const std::string& name) const;
  std::vector<ModelDesc> GetModelDescs(const std::string& name) const;
  bool HasModel(const std::string& name) const;
  std::unique_ptr<Model> CreateModel(const ModelDesc& model_desc,
                                     Shape input_shape, size_t batch_size = 1);

 private:
  // Mean colors, in BGR order.
  cv::Scalar mean_colors_;
  std::unordered_map<std::string, std::vector<ModelDesc>> model_descs_;
};

#endif  // SAF_MODEL_MODEL_MANAGER_H_
