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

// Intel Inference Engine base

#ifndef SAF_CVSDK_CVSDK_BASE_H_
#define SAF_CVSDK_CVSDK_BASE_H_

#include <boost/noncopyable.hpp>

#include <ie_extension.h>
#include <ie_iexecutable_network.hpp>
#include <ie_plugin_ptr.hpp>
#include <inference_engine.hpp>

#include "model/model.h"

class CVSDKBase : public boost::noncopyable {
 public:
  CVSDKBase() {}
  virtual ~CVSDKBase(){};
  void Initialize(ModelDesc& model_desc_);

 protected:
  InferenceEngine::InferenceEnginePluginPtr engine_;
  InferenceEngine::IExecutableNetwork::Ptr network_;
  InferenceEngine::CNNNetReader network_builder_;
  std::string network_input_name_;
  std::string network_output_name_;
  InferenceEngine::BlobMap input_blobs_;
  InferenceEngine::BlobMap output_blobs_;
  InferenceEngine::TBlob<unsigned char>::Ptr input_;
};

#endif  // SAF_CVSDK_CVSDK_BASE_H_
