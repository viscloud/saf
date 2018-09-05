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

#include "common/context.h"

#include <ie_plugin_config.hpp>
#include <vpu/vpu_plugin_config.hpp>

#include <ie_extension.h>
#include "cvsdk/common.hpp"
#include "cvsdk/ext_list.hpp"

#include "model/model_manager.h"

#include "cvsdk/cvsdk_base.h"

using namespace InferenceEngine::details;
using namespace InferenceEngine;

void CVSDKBase::Initialize(ModelDesc& model_desc) {
  LOG(INFO) << "InferenceEngine: "
            << InferenceEngine::GetInferenceEngineVersion();
  LOG(INFO) << "Loading plugin";
  InferenceEngine::PluginDispatcher dispatcher({"", "", ""});

  /** Loading plugin for device **/
  // Set CVSDK backend
  int desired_device_number = model_desc.GetDevice()
                                  ? *(model_desc.GetDevice())
                                  : Context::GetContext().GetInt(DEVICE_NUMBER);
  if (desired_device_number == DEVICE_NUMBER_CPU_ONLY) {
    engine_ = dispatcher.getPluginByDevice("CPU");
  } else if (desired_device_number == DEVICE_NUMBER_MYRIAD) {
    engine_ = dispatcher.getPluginByDevice("MYRIAD");
  } else {
    engine_ = dispatcher.getPluginByDevice("GPU");
  }

  // Load CPU extension as needed.
  InferencePlugin plugin(engine_);
  if (desired_device_number == DEVICE_NUMBER_CPU_ONLY) {
    plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
  }

  // Print OpenVINO plugin version.
  const PluginVersion* pluginVersion;
  engine_->GetVersion((const Version*&)pluginVersion);
  LOG(INFO) << "OpenVINO plugin version: " << pluginVersion;

  // Load network model
  network_builder_.ReadNetwork(model_desc.GetModelDescPath());
  network_builder_.ReadWeights(model_desc.GetModelParamsPath());

  // Prepare input blobs
  InputsDataMap inputsInfo(network_builder_.getNetwork().getInputsInfo());

  if (inputsInfo.size() != 1) {
    throw std::logic_error("Sample supports topologies only with 1 input");
  }

  network_input_name_ = inputsInfo.begin()->first;
  auto firstInputInfo = inputsInfo.begin()->second;

  // Set batch size to 1
  // TODO: Enable batching
  auto inputDims = firstInputInfo->getDims();
  if (inputDims.back() != 1) {
    network_builder_.getNetwork().setBatchSize(1);
    inputDims = firstInputInfo->getDims();
  }

  // Creat input blobs
  Precision inputPrecision = Precision::U8;
  firstInputInfo->setInputPrecision(inputPrecision);
  input_ = make_shared_blob<unsigned char, SizeVector>(
      inputPrecision, firstInputInfo->getDims());
  input_->allocate();
  input_blobs_[network_input_name_] = input_;

  // Prepare output blobs
  OutputsDataMap outputsInfo(network_builder_.getNetwork().getOutputsInfo());

  if (outputsInfo.size() != 1) {
    throw std::logic_error(
        "This sample accepts networks having only one output");
  }

  network_output_name_ = outputsInfo.begin()->first;
  auto firstOutputInfo = outputsInfo.begin()->second;

  if (!firstOutputInfo) {
    throw std::logic_error("output data pointer is not valid");
  }

  // Creat output blobs
  TBlob<float>::Ptr output;
  Precision outputPrecision = Precision::FP32;
  firstOutputInfo->precision = outputPrecision;
  output = make_shared_blob<float, SizeVector>(outputPrecision,
                                               firstOutputInfo->dims);
  output->allocate();
  output_blobs_[network_output_name_] = output;

  // Load network to the plugin
  ResponseDesc resp;
  std::map<std::string, std::string> config;
  // TODO: Uncomment the following whenever ready
  //  if (desired_device_number == DEVICE_NUMBER_MYRIAD) {
  //    config[VPU_CONFIG_KEY(HW_STAGES_OPTIMIZATION)] = CONFIG_VALUE(YES);
  //  }
  //  StatusCode status = engine_->LoadNetwork(network_builder_.getNetwork(),
  //  &resp);
  StatusCode status = engine_->LoadNetwork(
      network_, network_builder_.getNetwork(), config, &resp);
  if (status != InferenceEngine::OK) {
    throw std::logic_error(resp.msg);
  }
}
