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

// Python API

#include "api/python/safpy.h"

#include <regex>

#include <glog/logging.h>
#include <gst/gst.h>
#include <numpy/arrayobject.h>
#include <boost/python.hpp>
#include <pyboostcvconverter/src/pyboost_cv3_converter.cpp>
#include "rttr/registration"

#include "camera/camera_manager.h"
#include "model/model_manager.h"
#include "operator/detectors/object_detector.h"
#include "operator/extractors/feature_extractor.h"
#include "operator/image_classifier.h"
#include "operator/image_transformer.h"
#include "operator/matchers/object_matcher.h"
#include "operator/receivers/receiver.h"
#include "operator/senders/sender.h"
#include "operator/trackers/object_tracker.h"
#include "operator/writers/writer.h"
#include "pipeline/pipeline.h"

#define PY_ARRAY_UNIQUE_SYMBOL pbcvt_ARRAY_API

// Private constructor for singleton.
SafPython::SafPython(const std::string& config_path)
    : isGoogleLogging(false), is_initialized_(false) {
  Init(config_path);
}

SafPython* SafPython::GetInstance(const std::string& config) {
  static SafPython* safPython = new SafPython(config);
  return safPython;
}

void SafPython::StopAndClean() {
  Stop();
  Clean();
}

void SafPython::Stop() {
  for (const auto& op : ops_) {
    if (op->IsStarted()) op->Stop();
    while (op->IsStarted()) {
      op->Stop();
    }
  }
}
void SafPython::Clean() {
  for (const auto& reader : readers_) {
    reader->Stop();
    reader->Unsubscribe();
  }
  readers_.clear();
  ops_.clear();
  if (isGoogleLogging) {
    google::ShutdownGoogleLogging();
    isGoogleLogging = false;
  }
}

void SafPython::Start() {
  for (auto ops_it = SafPython::ops_.rbegin(); ops_it != SafPython::ops_.rend();
       ++ops_it) {
    (*ops_it)->Start();
  }
}

std::shared_ptr<Operator> SafPython::CreateCamera(
    const std::string& camera_name) {
  auto camera = CameraManager::GetInstance().GetCamera(camera_name);
  return camera;
}

std::shared_ptr<Operator> SafPython::CreateTransformerUsingModel(
    const std::string& model_name, int num_channels, int angle) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(num_channels, model_desc.GetInputWidth(),
                    model_desc.GetInputHeight());
  return std::make_shared<ImageTransformer>(input_shape, true, angle);
}

std::shared_ptr<Operator> SafPython::CreateTransformerWithValues(
    int numberOfChannels, int width, int height, int angle) {
  return std::make_shared<ImageTransformer>(
      Shape(numberOfChannels, width, height), true, angle);
}

std::shared_ptr<Operator> SafPython::CreateClassifier(
    const std::string& model_name, int num_channels, int num_labels) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(num_channels, model_desc.GetInputWidth(),
                    model_desc.GetInputHeight());
  return std::make_shared<ImageClassifier>(model_desc, input_shape, num_labels);
}

std::shared_ptr<Operator> SafPython::CreateDetector(
    const std::string& detector_type, const std::string& model_name,
    float detector_confidence_threshold, float detector_idle_duration,
    int face_min_size, const boost::python::list& targets, size_t batch_size) {
  auto model_descs = ModelManager::GetInstance().GetModelDescs(model_name);
  std::shared_ptr<ObjectDetector> detector;
  std::set<std::string> set;
  for (int i = 0; i < len(targets); i++) {
    boost::python::extract<std::string> target(targets[i]);
    set.insert((std::string)target);
  }
  detector = std::make_shared<ObjectDetector>(
      detector_type, model_descs, batch_size, detector_confidence_threshold,
      detector_idle_duration, set, face_min_size);
  return detector;
}

std::shared_ptr<Operator> SafPython::CreateTracker(
    const std::string& tracker_type) {
  return std::make_shared<ObjectTracker>(tracker_type);
}

std::shared_ptr<Operator> SafPython::CreateExtractor(
    const std::string& extractor_type, const std::string& extractor_model,
    size_t batch_size) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(extractor_model);
  return std::make_shared<FeatureExtractor>(model_desc, batch_size,
                                            extractor_type);
}

std::shared_ptr<Operator> SafPython::CreateMatcher(
    const std::string& matcher_type, const std::string& matcher_model,
    float matcher_distance_threshold, size_t batch_size) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(matcher_model);
  return std::make_shared<ObjectMatcher>(
      matcher_type, batch_size, matcher_distance_threshold, model_desc);
}

std::shared_ptr<Operator> SafPython::CreateWriter(const std::string& target,
                                                  const std::string& uri,
                                                  size_t batch_size) {
  return std::make_shared<Writer>(target, uri, batch_size);
}

std::shared_ptr<Operator> SafPython::CreateSender(
    const std::string& endpoint, const std::string& package_type,
    size_t batch_size) {
  return std::make_shared<Sender>(endpoint, package_type, batch_size);
}

std::shared_ptr<Operator> SafPython::CreateReceiver(
    const std::string& endpoint, const std::string& package_type,
    const std::string& aux) {
  return std::make_shared<Receiver>(endpoint, package_type, aux);
}

std::shared_ptr<Operator> SafPython::CreateOperatorByName(
    const std::string& op_type, boost::python::dict& paramsDict) {
  FactoryParamsType params;
  boost::python::list keys = paramsDict.keys();
  for (int i = 0; i < len(keys); i++) {
    boost::python::extract<std::string> key(keys[i]);
    if (!key.check()) {
      LOG(ERROR) << "Error Key" << std::endl;
      exit(1);
    }
    std::string keyStr = key;
    boost::python::extract<std::string> value(paramsDict[keyStr]);
    if (!value.check()) {
      LOG(ERROR) << "Error value" << std::endl;
      exit(1);
    }
    std::string valueStr = value;
    params[keyStr] = valueStr;
  }

  using namespace rttr;
  type class_type = type::get_by_name(op_type);
  if (class_type) {
    variant opVar = class_type.create();
    variant createPtr = class_type.invoke("Create", opVar, {params});
    return createPtr.get_value<std::shared_ptr<Operator>>();
  } else {
    LOG(FATAL) << op_type << " operator hasn't been registered with RTTR.";
  }
}

std::shared_ptr<Readerpy> SafPython::CreateReader(
    std::shared_ptr<Operator> op, const std::string output_name) {
  auto reader = std::make_shared<Readerpy>(op, output_name);
  readers_.push_back(reader);
  return reader;
}

void SafPython::LoadGraph(boost::python::dict& graph) {
  boost::python::list src_list = graph.keys();
  for (int i = 0; i < len(src_list); i++) {
    boost::python::extract<std::string> src(src_list[i]);
    if (!src.check()) {
      LOG(FATAL) << "Error Key" << std::endl;
    }
    std::string srcStr = src;
    boost::python::extract<boost::python::list> dstList(graph[srcStr]);
    if (!dstList.check()) {
      LOG(FATAL) << "Error value" << std::endl;
    }
    boost::python::extract<std::shared_ptr<Operator>> srcPtr(
        boost::python::eval(srcStr.c_str()));
    boost::python::list list = dstList;
    for (int j = 0; j < len(list); j++) {
      boost::python::extract<std::string> dst(list[j]);
      std::string dstStr = (std::string)dst;
      std::size_t posInp = dstStr.find(":");
      std::string input_name = std::string();
      std::string output_name = std::string();
      if (posInp != std::string::npos) {
        std::size_t posOut = dstStr.find(":", posInp + 1);
        if (posOut != std::string::npos) {
          input_name = dstStr.substr(posInp + 1, dstStr.length() - posOut - 1);
          output_name = dstStr.substr(posOut + 1);
          dstStr = dstStr.substr(0, posInp);
        }
      }
      boost::python::extract<std::shared_ptr<Operator>> dstPtr(
          boost::python::eval(dstStr.c_str()));
      if (!(std::find(ops_.begin(), ops_.end(),
                      (std::shared_ptr<Operator>)srcPtr) != ops_.end())) {
        AddOperator(srcPtr);
      }
      if (!(std::find(ops_.begin(), ops_.end(),
                      (std::shared_ptr<Operator>)dstPtr) != ops_.end())) {
        AddOperator(dstPtr);
      }
      if (input_name.empty()) {
        ConnectToOperator(srcPtr, dstPtr);
      } else {
        ConnectToOperator(srcPtr, dstPtr, input_name, output_name);
      }
    }
  }
}

void SafPython::AddCamera(const std::string& camera_name) {
  auto camera = CameraManager::GetInstance().GetCamera(camera_name);
  ops_.push_back(camera);
}

void SafPython::AddTransformerUsingModel(const std::string& model_name,
                                         int num_channels, int angle) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(num_channels, model_desc.GetInputWidth(),
                    model_desc.GetInputHeight());
  auto transformer =
      std::make_shared<ImageTransformer>(input_shape, true, angle);
  transformer->SetSource("input", ops_.at(ops_.size() - 1)->GetSink("output"));
  ops_.push_back(transformer);
}

void SafPython::AddTransformerWithValues(int numberOfChannels, int width,
                                         int height, int angle) {
  auto transformer = std::make_shared<ImageTransformer>(
      Shape(numberOfChannels, width, height), true, angle);
  transformer->SetSource("input", ops_.at(ops_.size() - 1)->GetSink("output"));
  ops_.push_back(transformer);
}

void SafPython::AddClassifier(const std::string& model_name, int num_channels) {
  auto model_desc = ModelManager::GetInstance().GetModelDesc(model_name);
  Shape input_shape(num_channels, model_desc.GetInputWidth(),
                    model_desc.GetInputHeight());
  auto classifier =
      std::make_shared<ImageClassifier>(model_desc, input_shape, 1);
  classifier->SetSource("input", ops_.at(ops_.size() - 1)->GetSink("output"));
  ops_.push_back(classifier);
}

void SafPython::AddOperator(std::shared_ptr<Operator> op) {
  // TODO: Check if operator has been already added.
  ops_.push_back(op);
  is_end_op_.push_back(true);
}

// Legacy function. It will be removed. Use Graph or Add and Connect.
void SafPython::AddOperatorAndConnectToLast(std::shared_ptr<Operator> opDst) {
  if (ops_.size() == 0) {
    ops_.push_back(opDst);
    is_end_op_.push_back(true);
    return;
  }
  auto& opSrc = ops_.at(ops_.size() - 1);
  if (dynamic_cast<ObjectDetector*>(opDst.get()) != nullptr) {
    if (dynamic_cast<Camera*>(opSrc.get()) != nullptr) {
      Camera* camera = (Camera*)opSrc.get();
      ObjectDetector* detector = (ObjectDetector*)opDst.get();
      detector->SetInputStream(0, camera->GetStream());  // TODO: Detectors
      // TODO: it can only take one input. Fix this.
    }
  } else {
    // TODO: Allow for multiple inputs and named outputs
    opDst->SetSource("input", ops_.at(ops_.size() - 1)->GetSink("output"));
    is_end_op_.at(ops_.size() - 1) = false;
  }
  ops_.push_back(opDst);
  is_end_op_.push_back(true);
}

void SafPython::ConnectToOperator(std::shared_ptr<Operator> opSrc,
                                  std::shared_ptr<Operator> opDst,
                                  const std::string input_name,
                                  const std::string output_name) {
  if (dynamic_cast<Writer*>(opDst.get()) != nullptr) {
    auto itr = std::find(ops_.begin(), ops_.end(), opDst);
    if (itr == ops_.end()) {
      LOG(FATAL) << " Error connecting operators. Destination operator "
                    "doesn't exist.";
    } else {
      is_end_op_[itr - ops_.begin()] = false;
    }
  }
  auto itr = std::find(ops_.begin(), ops_.end(), opSrc);
  if (itr == ops_.end()) {
    LOG(FATAL) << " Error connecting operators. Source operator doesn't exist.";
  } else {
    is_end_op_[itr - ops_.begin()] = false;
  }
  opDst->SetSource(input_name, opSrc->GetSink(output_name));
}

void SafPython::CreatePipeline(const std::string& pipeline_filepath) {
  std::ifstream i(pipeline_filepath);
  nlohmann::json json;
  i >> json;
  std::shared_ptr<Pipeline> pipeline = Pipeline::ConstructPipeline(json);
  auto ops = pipeline->GetOperators();
  for (std::pair<std::string, std::shared_ptr<Operator>> op : ops) {
    AddOperator(op.second);
    is_end_op_.at(is_end_op_.size() - 1) = false;
  }
  is_end_op_.at(is_end_op_.size() - 1) = true;
}

void SafPython::Visualize(std::shared_ptr<Operator> op,
                          const std::string output_name) {
  StreamReader* reader;
  if (op == nullptr) {
    std::vector<std::shared_ptr<Operator>> results;
    for (std::size_t i = 0; i < ops_.size(); i++) {
      if (is_end_op_[i] == true) {
        results.push_back(ops_[i]);
      }
    }
    op = results[results.size() - 1];
  }

  reader = op->GetSink(output_name)->Subscribe();

  cv::Scalar rect_color(0, 0, 255);
  std::vector<Rect> bboxes;
  std::vector<std::string> tags;
  while (true) {
    auto frame = reader->PopFrame();
    auto img = frame->GetValue<cv::Mat>("original_image");

    // Show bounding boxes if they are available.
    bool show_boxes = true;
    try {
      bboxes = frame->GetValue<std::vector<Rect>>("bounding_boxes");
    } catch (const std::runtime_error& e) {
      show_boxes = false;
    }
    bool show_labels = true;
    try {
      // TODO: I have disabled the display of tags because of conflicts between
      // detector and classifier.
      tags = frame->GetValue<std::vector<std::string>>("tags");
    } catch (const std::runtime_error& oor) {
      show_labels = false;
    }
    // OBJECT DETECTION
    if (show_boxes) {
      for (size_t j = 0; j < bboxes.size(); ++j) {
        cv::Point top_left_pt(bboxes[j].px, bboxes[j].py);
        cv::Point bottom_right_pt(bboxes[j].px + bboxes[j].width,
                                  bboxes[j].py + bboxes[j].height);
        cv::rectangle(img, top_left_pt, bottom_right_pt, rect_color, 4);
        cv::Point bottom_left_pt(bboxes[j].px, bboxes[j].py + bboxes[j].height);
        std::ostringstream object_labels;
        //        object_labels << tags[j];
        if (frame->Count("uuids") > 0) {
          auto uuids = frame->GetValue<std::vector<std::string>>("uuids");
          std::size_t pos = uuids[j].size();
          auto shared_uuid = uuids[j].substr(pos - 5);
          object_labels << ": " << shared_uuid;
        }
        cv::Size text_size = cv::getTextSize(object_labels.str().c_str(),
                                             cv::FONT_HERSHEY_SIMPLEX, 1, 2, 0);

        cv::rectangle(
            img, bottom_left_pt + cv::Point(0, 0),
            bottom_left_pt + cv::Point(text_size.width, -text_size.height),
            rect_color, CV_FILLED);
        cv::putText(img, object_labels.str(), bottom_left_pt - cv::Point(0, 0),
                    cv::FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 0, 0), 2, 8);
      }
    }
    // CLASSIFIER
    if (!show_boxes && show_labels) {
      // 'tags' are used by both detector and classifier. We should have
      // different keys, e.g. 'tags' for detector, 'label' for classifier.
      auto probs = frame->GetValue<std::vector<double>>("probabilities");
      auto prob_percent = probs.front() * 100;
      auto tag = tags.front();
      std::regex re(".+? (.+)");
      std::smatch results;
      std::string tag_name;
      if (!std::regex_match(tag, results, re)) {
        tag_name = tag;
      } else {
        tag_name = results[1];
      }
      std::ostringstream label;
      label.precision(2);
      label << prob_percent << "% - " << tag_name;
      auto label_string = label.str();
      auto font_scale = 2.0;
      cv::Point label_point(25, 50);
      cv::Scalar label_color(200, 200, 250);
      cv::Scalar outline_color(0, 0, 0);
      cv::putText(img, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                  font_scale, outline_color, 8, CV_AA);
      cv::putText(img, label_string, label_point, CV_FONT_HERSHEY_PLAIN,
                  font_scale, label_color, 2, CV_AA);
    }

    // Show face_landmarks if they are available
    if (frame->Count("face_landmarks") > 0) {
      auto face_landmarks =
          frame->GetValue<std::vector<FaceLandmark>>("face_landmarks");
      for (const auto& m : face_landmarks) {
        for (int j = 0; j < 5; j++)
          cv::circle(img, cv::Point(m.x[j], m.y[j]), 1, cv::Scalar(255, 255, 0),
                     5);
      }
    }
    cv::imshow("Output", img);
    if (cv::waitKey(5) == 'q') break;
  }  // While loop
  cv::destroyAllWindows();
}

void SafPython::Init(const std::string& config_path) {
  gst_init(NULL, NULL);
  google::InitGoogleLogging("Safpy");
  isGoogleLogging = true;
  FLAGS_alsologtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  Context::GetContext().SetConfigDir(config_path);
  Context::GetContext().Init();
  is_initialized_ = true;
}

void SafPython::SetDeviceNumber(int device_number) {
  if (is_initialized_) {
    Context::GetContext().SetInt(DEVICE_NUMBER, device_number);
    LOG(INFO) << "Device number set to " << device_number;
  } else {
    LOG(FATAL)
        << "Device number cannot be set if context hasn't been initialized";
  }
}

std::shared_ptr<SafPython> GetInstance(
    const std::string& config_path = "./config") {
  return std::shared_ptr<SafPython>(SafPython::GetInstance(config_path));
}

RTTR_REGISTRATION {
  using namespace rttr;
  registration::class_<ImageClassifier>("ImageClassifier")
      .method("Create", &ImageClassifier::Create);
  registration::class_<ImageTransformer>("ImageTransformer")
      .method("Create", &ImageTransformer::Create);
  registration::class_<Sender>("Sender").method("Create", &Sender::Create);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(GetInstance_overloads, GetInstance, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateTransformerUsingModel_overloads,
                                       SafPython::CreateTransformerUsingModel,
                                       1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateClassifier_overloads,
                                       SafPython::CreateClassifier, 1, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateDetector_overloads,
                                       SafPython::CreateDetector, 2, 7);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateExtractor_overloads,
                                       SafPython::CreateExtractor, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateMatcher_overloads,
                                       SafPython::CreateMatcher, 3, 4);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateWriter_overloads,
                                       SafPython::CreateWriter, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateSender_overloads,
                                       SafPython::CreateSender, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateReceiver_overloads,
                                       SafPython::CreateReceiver, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(AddTransformerUsingModel_overloads,
                                       SafPython::AddTransformerUsingModel, 1,
                                       2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(AddClassifier_overloads,
                                       SafPython::AddClassifier, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ConnectToOperator_overloads,
                                       SafPython::ConnectToOperator, 2, 4);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CreateReader_overloads,
                                       SafPython::CreateReader, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(Visualize_overloads,
                                       SafPython::Visualize, 0, 2);

BOOST_PYTHON_MODULE(safpy) {
  using namespace boost::python;

  Py_Initialize();
  import_array();

  def("GetInstance", GetInstance, GetInstance_overloads());
  def("SetDeviceNumber", &SafPython::SetDeviceNumber);

  class_<SafPython, std::shared_ptr<SafPython>, boost::noncopyable>("Saf",
                                                                    no_init)
      .def("StopAndClean", &SafPython::StopAndClean)
      .def("Start", &SafPython::Start)
      .def("Stop", &SafPython::Stop)
      .def("Clean", &SafPython::Clean)
      .def("Camera", &SafPython::CreateCamera)
      .def("Transformer", &SafPython::CreateTransformerUsingModel,
           CreateTransformerUsingModel_overloads())
      .def("Transformer", &SafPython::CreateTransformerWithValues)
      .def("Classifier", &SafPython::CreateClassifier,
           CreateClassifier_overloads())
      .def("Detector", &SafPython::CreateDetector,
           CreateDetector_overloads(
               (boost::python::arg("detector_type"),
                boost::python::arg("model_name"),
                boost::python::arg("detector_confidence_threshold") = 0.1f,
                boost::python::arg("detector_idle_duration") = 0.f,
                boost::python::arg("face_min_size") = 40,
                boost::python::arg("targets") = boost::python::list(),
                boost::python::arg("batch_size") = 1)))
      .def("Tracker", &SafPython::CreateTracker)
      .def("Extractor", &SafPython::CreateExtractor,
           CreateExtractor_overloads())
      .def("Matcher", &SafPython::CreateMatcher, CreateMatcher_overloads())
      .def("Writer", &SafPython::CreateWriter, CreateWriter_overloads())
      .def("Sender", &SafPython::CreateSender, CreateSender_overloads())
      .def("Receiver", &SafPython::CreateReceiver, CreateReceiver_overloads())
      .def("Operator", &SafPython::CreateOperatorByName)
      .def("Reader", &SafPython::CreateReader,
           CreateReader_overloads())  // We can use both Reader and Subscribe.
      .def("Subscribe", &SafPython::CreateReader, CreateReader_overloads())
      .def("LoadGraph", &SafPython::LoadGraph)
      .def("AddCamera", &SafPython::AddCamera)
      .def("AddTransformer", &SafPython::AddTransformerUsingModel,
           AddTransformerUsingModel_overloads())
      .def("AddTransformer", &SafPython::AddTransformerWithValues)
      .def("AddClassifier", &SafPython::AddClassifier,
           AddClassifier_overloads())
      .def("Add", &SafPython::AddOperator)
      .def("AddAndConnect", &SafPython::AddOperatorAndConnectToLast)
      .def("Connect", &SafPython::ConnectToOperator,
           ConnectToOperator_overloads())
      .def("Pipeline", &SafPython::CreatePipeline)
      .def("Visualize", &SafPython::Visualize, Visualize_overloads());

  // Subscribe <=> Reader
  class_<Readerpy, std::shared_ptr<Readerpy>>(
      "Readerpy", init<std::shared_ptr<Operator>, const std::string>())
      .def("PopFrame", &Readerpy::PopFrame)
      .def("GetPushFps", &Readerpy::GetPushFps);

  // Frame
  class_<Framepy, std::shared_ptr<Framepy>>("Framepy",
                                            init<std::shared_ptr<Frame>>())
      .def("GetValue", &Framepy::GetValue);

  class_<cv::Mat>("Mat");

  class_<Frame, std::shared_ptr<Frame>>("Frame").def("GetValue",
                                                     &Frame::GetValue<cv::Mat>);
  // Just in case. I don't think we will use it.
  class_<CameraManager, boost::noncopyable>("CameraManager", no_init)
      .def("GetCameraAlt", &CameraManager::GetCamera)
      .def("CameraManagerInstance", &CameraManager::GetInstance,
           return_value_policy<reference_existing_object>())
      .staticmethod("CameraManagerInstance");

  // Abstract classes.
  // I only have Camera here because it is required in GetCameraAlt. Otherwise,
  // it is not needed.
  class_<Camera, std::shared_ptr<Camera>, boost::noncopyable>(
      "_Camera", no_init);  // Using Operator instead
  class_<Operator, std::shared_ptr<Operator>, boost::noncopyable>("_Operator",
                                                                  no_init);
}
