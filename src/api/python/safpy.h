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

// Python API for SAF
// It allows for the creation of SAF pipelines in Python.

#ifndef SAF_API_PYTHON_SAFPY_H_
#define SAF_API_PYTHON_SAFPY_H_

#include "api/python/framepy.h"
#include "api/python/readerpy.h"

class SafPython {
 public:
  static SafPython* GetInstance(const std::string& config);
  void StopAndClean();
  void Stop();
  void Clean();
  void Start();

  // Operators
  std::shared_ptr<Operator> CreateCamera(const std::string& camera_name);
  std::shared_ptr<Operator> CreateTransformerUsingModel(
      const std::string& model_name, int num_channels = 3, int angle = 0);
  std::shared_ptr<Operator> CreateTransformerWithValues(int numberOfChannels,
                                                        int width, int height,
                                                        int angle = 0);
  std::shared_ptr<Operator> CreateClassifier(const std::string& model_name,
                                             int num_channels = 3,
                                             int num_labels = 1);
  std::shared_ptr<Operator> CreateDetector(
      const std::string& detector_type, const std::string& model_name,
      float detector_confidence_threshold = 0.1f,
      float detector_idle_duration = 0.f, int face_min_size = 40,
      const boost::python::list& targets = boost::python::list(),
      size_t batch_size = 1);
  std::shared_ptr<Operator> CreateTracker(const std::string& tracker_type);
  std::shared_ptr<Operator> CreateExtractor(const std::string& extractor_type,
                                            const std::string& extractor_model,
                                            size_t batch_size = 1);
  std::shared_ptr<Operator> CreateMatcher(const std::string& matcher_type,
                                          const std::string& matcher_model,
                                          float matcher_distance_threshold,
                                          size_t batch_size = 1);
  std::shared_ptr<Operator> CreateWriter(const std::string& target,
                                         const std::string& uri,
                                         size_t batch_size = 1);
  std::shared_ptr<Operator> CreateSender(const std::string& endpoint,
                                         const std::string& package_type,
                                         size_t batch_size = 1);
  std::shared_ptr<Operator> CreateReceiver(const std::string& endpoint,
                                           const std::string& package_type,
                                           const std::string& aux = "");
  std::shared_ptr<Operator> CreateOperatorByName(
      const std::string& operator_name, boost::python::dict& params);
  std::shared_ptr<Readerpy> CreateReader(std::shared_ptr<Operator> op,
                                         const std::string output = "output");

  // Graph
  void LoadGraph(boost::python::dict& graph);

  // Create pipeline programmatically
  void AddCamera(const std::string& camera_name);
  void AddTransformerUsingModel(const std::string& model_name,
                                int num_channels = 3, int angle = 0);
  void AddTransformerWithValues(int numberOfChannels, int width, int height,
                                int angle = 0);
  void AddClassifier(const std::string& model_name, int num_channels = 3);
  void AddOperator(std::shared_ptr<Operator> op);
  void AddOperatorAndConnectToLast(std::shared_ptr<Operator> op);
  void ConnectToOperator(std::shared_ptr<Operator> opSrc,
                         std::shared_ptr<Operator> op_dst,
                         const std::string input_name = "input",
                         const std::string output_name = "output");

  void CreatePipeline(const std::string& pipeline_filepath);

  void Visualize(std::shared_ptr<Operator> op = nullptr,
                 const std::string output_name = "output");
  void SetDeviceNumber(int device_number);

  // Singleton
  SafPython(SafPython const&) = delete;
  void operator=(SafPython const&) = delete;

 private:
  std::vector<std::shared_ptr<Operator>> ops_;
  std::vector<bool> is_end_op_;
  std::vector<std::shared_ptr<Readerpy>> readers_;
  std::atomic<bool> isGoogleLogging;
  bool is_initialized_;

  SafPython(const std::string& config_path);

  void Init(const std::string& config_path);
};

#endif  // SAF_API_PYTHON_SAFPY_H_
