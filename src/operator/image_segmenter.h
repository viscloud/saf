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

#ifndef SAF_OPERATOR_IMAGE_SEGMENTER_H_
#define SAF_OPERATOR_IMAGE_SEGMENTER_H_

#include "common/types.h"
#include "model/model.h"
#include "operator/operator.h"

class ImageSegmenter : public Operator {
 public:
  ImageSegmenter(const ModelDesc& model_desc, Shape input_shape);

  static std::shared_ptr<ImageSegmenter> Create(
      const FactoryParamsType& params);

 protected:
  virtual bool Init() override;
  virtual bool OnStop() override;
  virtual void Process() override;

 private:
  std::unique_ptr<Model> model_;
  ModelDesc model_desc_;
  Shape input_shape_;
  cv::Mat mean_image_;
};

#endif  // SAF_OPERATOR_IMAGE_SEGMENTER_H_
