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

// Intel Inference Engine model

#ifndef SAF_MODEL_CVSDK_MODEL_H_
#define SAF_MODEL_CVSDK_MODEL_H_

#include <unordered_map>

#include <ie_extension.h>
#include <ie_iexecutable_network.hpp>
#include <ie_plugin_ptr.hpp>
#include <inference_engine.hpp>
#include <opencv2/opencv.hpp>

#include "cvsdk/cvsdk_base.h"
#include "model/model.h"

class CVSDKModel : public Model, public CVSDKBase {
 public:
  CVSDKModel(const ModelDesc& model_desc, Shape input_shape,
             size_t batch_size = 1);
  virtual void Load() override;
  virtual std::unordered_map<std::string, std::vector<cv::Mat>> Evaluate(
      const std::unordered_map<std::string, std::vector<cv::Mat>>& input_map,
      const std::vector<std::string>& output_layer_names) override;

 private:
  cv::Mat BlobToMat2d(InferenceEngine::TBlob<float>& input,
                      int batch_idx) const;
  cv::Mat GetLayerOutput(const std::string& layer_name, int batch_idx,
                         const InferenceEngine::BlobMap& outputBlobs) const;
};

#endif  // SAF_MODEL_CVSDK_MODEL_H_
