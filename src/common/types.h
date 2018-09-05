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

#ifndef SAF_COMMON_TYPES_H_
#define SAF_COMMON_TYPES_H_

#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_map>

#include <glog/logging.h>
#include <boost/serialization/access.hpp>
#include <json/src/json.hpp>

#include "common/serialization.h"

/**
 * @brief 3-D shape structure
 */
struct Shape {
  Shape() : channel(0), width(0), height(0){};
  Shape(int c, int w, int h) : channel(c), width(w), height(h){};
  Shape(int w, int h) : channel(1), width(w), height(h){};
  /**
   * @brief Return volumn (size) of the shape object
   */
  size_t GetSize() const { return (size_t)channel * width * height; }
  // Number of channels
  int channel;
  // Width
  int width;
  // Height
  int height;
};

/**
 * @brief Rectangle
 */
struct Rect {
  Rect() : px(0), py(0), width(0), height(0){};
  Rect(int x, int y, int w, int h) : px(x), py(y), width(w), height(h){};
  Rect(nlohmann::json j) {
    try {
      nlohmann::json rect_j = j.at("Rect");
      px = rect_j.at("px").get<int>();
      py = rect_j.at("py").get<int>();
      width = rect_j.at("width").get<int>();
      height = rect_j.at("height").get<int>();
    } catch (std::out_of_range& e) {
      LOG(FATAL) << "Malformed Rect JSON: " << j.dump() << "\n" << e.what();
    }
  }
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive&, const unsigned int) {}

  bool operator==(const Rect& rhs) const {
    return (px == rhs.px) && (py == rhs.py) && (width == rhs.width) &&
           (height == rhs.height);
  }

  nlohmann::json ToJson() const {
    nlohmann::json rect_j;
    rect_j["px"] = px;
    rect_j["py"] = py;
    rect_j["width"] = width;
    rect_j["height"] = height;
    nlohmann::json j;
    j["Rect"] = rect_j;
    return j;
  }

  // The top left point of the rectangle
  int px;
  int py;
  // The width and height of the rectangle
  int width;
  int height;
};

/**
 * @brief Face landmark
 */
struct FaceLandmark {
  FaceLandmark() {
    x.resize(5);
    y.resize(5);
  }

  FaceLandmark(nlohmann::json j) {
    try {
      nlohmann::json face_j = j.at("FaceLandmark");
      x = face_j.at("px").get<std::vector<float>>();
      y = face_j.at("py").get<std::vector<float>>();
    } catch (std::out_of_range& e) {
      LOG(FATAL) << "Malformed FaceLandark JSON: " << j.dump() << "\n"
                 << e.what();
    }
  }

  nlohmann::json ToJson() const {
    nlohmann::json face_j;
    face_j["x"] = x;
    face_j["y"] = y;
    nlohmann::json j;
    j["FaceLandmark"] = face_j;
    return j;
  }

  template <class Archive>
  void serialize(Archive&, const unsigned int) {}

  std::vector<float> x;
  std::vector<float> y;
};

/**
 * @brief Point feature
 */
struct PointFeature {
  PointFeature(const cv::Point& p, const std::vector<float>& f)
      : point(p), face_feature(f) {}
  template <class Archive>
  void serialize(Archive&, const unsigned int) {}
  cv::Point point;
  std::vector<float> face_feature;
};

/**
 * @brief Prediction result, a string label and a confidence score
 */
typedef std::pair<std::string, float> Prediction;

class Stream;
typedef std::shared_ptr<Stream> StreamPtr;
class Camera;
typedef std::shared_ptr<Camera> CameraPtr;
class Pipeline;
typedef std::shared_ptr<Pipeline> PipelinePtr;
class Frame;
typedef std::shared_ptr<Frame> FramePtr;
class Operator;
typedef std::shared_ptr<Operator> OperatorPtr;

typedef std::unordered_map<std::string, std::string> FactoryParamsType;

//// Model types
enum ModelType {
  MODEL_TYPE_INVALID = 0,
  MODEL_TYPE_CAFFE,
  MODEL_TYPE_TENSORFLOW,
  MODEL_TYPE_OPENCV,
  MODEL_TYPE_NCS,
  MODEL_TYPE_CVSDK,
  MODEL_TYPE_XQDA
};

//// Camera types
enum CameraType { CAMERA_TYPE_GST = 0, CAMERA_TYPE_PTGRAY, CAMERA_TYPE_VIMBA };

enum CameraModeType {
  CAMERA_MODE_0 = 0,
  CAMERA_MODE_1,
  CAMERA_MODE_2,
  CAMERA_MODE_3,
  CAMERA_MODE_COUNT,
  CAMERA_MODE_INVALID
};

enum CamereaFeatureType {
  CAMERA_FEATURE_INVALID = 0,
  CAMERA_FEATURE_EXPOSURE,
  CAMERA_FEATURE_GAIN,
  CAMERA_FEATURE_SHUTTER,
  CMAERA_FEATURE_IMAGE_SIZE,
  CAMERA_FEATURE_MODE
};

enum CameraImageSizeType {
  CAMERA_IMAGE_SIZE_INVALID = 0,
  CAMERA_IMAGE_SIZE_800x600,
  CAMERA_IMAGE_SIZE_1600x1200,
  CAMEAR_IMAGE_SIZE_1920x1080,
};

enum CameraPixelFormatType {
  CAMERA_PIXEL_FORMAT_INVALID = 0,
  CAMERA_PIXEL_FORMAT_RAW8,
  CAMERA_PIXEL_FORMAT_RAW12,
  CAMERA_PIXEL_FORMAT_MONO8,
  CAMERA_PIXEL_FORMAT_BGR,
  CAMERA_PIXEL_FORMAT_YUV411,
  CAMERA_PIXEL_FORMAT_YUV422,
  CAMERA_PIXEL_FORMAT_YUV444
};

std::string GetCameraPixelFormatString(CameraPixelFormatType pfmt);

//// Operator types
enum OperatorType {
  OPERATOR_TYPE_BINARY_FILE_WRITER = 0,
  OPERATOR_TYPE_BUFFER,
  OPERATOR_TYPE_CAMERA,
  OPERATOR_TYPE_COMPRESSOR,
  OPERATOR_TYPE_CUSTOM,
  OPERATOR_TYPE_WRITER,
  OPERATOR_TYPE_DISPLAY,
  OPERATOR_TYPE_ENCODER,
  OPERATOR_TYPE_FACE_TRACKER,
#ifdef USE_CAFFE
  OPERATOR_TYPE_FACENET,
#endif  // USE_CAFFE
  OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE,
  OPERATOR_TYPE_FLOW_CONTROL_EXIT,
#ifdef USE_RPC
  OPERATOR_TYPE_FRAME_RECEIVER,
  OPERATOR_TYPE_FRAME_SENDER,
#endif  // USE_RPC
  OPERATOR_TYPE_FRAME_PUBLISHER,
  OPERATOR_TYPE_FRAME_SUBSCRIBER,
  OPERATOR_TYPE_FRAME_WRITER,
  OPERATOR_TYPE_IMAGE_CLASSIFIER,
  OPERATOR_TYPE_IMAGE_SEGMENTER,
  OPERATOR_TYPE_IMAGE_TRANSFORMER,
  OPERATOR_TYPE_JPEG_WRITER,
  OPERATOR_TYPE_NEURAL_NET_EVALUATOR,
  OPERATOR_TYPE_OBJECT_DETECTOR,
  OPERATOR_TYPE_OBJECT_TRACKER,
  OPERATOR_TYPE_OBJECT_MATCHER,
  OPERATOR_TYPE_OPENCV_MOTION_DETECTOR,
  OPERATOR_TYPE_OPENCV_OPTICAL_FLOW,
  OPERATOR_TYPE_STRIDER,
  OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR,
  OPERATOR_TYPE_THROTTLER,
  OPERATOR_TYPE_SENDER,
  OPERATOR_TYPE_RECEIVER,
  OPERATOR_TYPE_FEATURE_EXTRACTOR,
  OPERATOR_TYPE_INVALID
};
// Returns the OperatorType enum value corresponding to the string.
inline OperatorType GetOperatorTypeByString(const std::string& type) {
  if (type == "BinaryFileWriter") {
    return OPERATOR_TYPE_BINARY_FILE_WRITER;
  } else if (type == "Buffer") {
    return OPERATOR_TYPE_BUFFER;
  } else if (type == "Camera") {
    return OPERATOR_TYPE_CAMERA;
  } else if (type == "Compressor") {
    return OPERATOR_TYPE_COMPRESSOR;
  } else if (type == "Custom") {
    return OPERATOR_TYPE_CUSTOM;
  } else if (type == "Writer") {
    return OPERATOR_TYPE_WRITER;
  } else if (type == "Display") {
    return OPERATOR_TYPE_DISPLAY;
  } else if (type == "GstVideoEncoder") {
    return OPERATOR_TYPE_ENCODER;
  } else if (type == "FaceTracker") {
    return OPERATOR_TYPE_FACE_TRACKER;
#ifdef USE_CAFFE
  } else if (type == "Facenet") {
    return OPERATOR_TYPE_FACENET;
  } else if (type == "FeatureExtractor") {
    return OPERATOR_TYPE_FEATURE_EXTRACTOR;
#endif  // USE_CAFFE
  } else if (type == "FlowControlEntrance") {
    return OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE;
  } else if (type == "FlowControlExit") {
    return OPERATOR_TYPE_FLOW_CONTROL_EXIT;
#ifdef USE_RPC
  } else if (type == "FrameReceiver") {
    return OPERATOR_TYPE_FRAME_RECEIVER;
  } else if (type == "FrameSender") {
    return OPERATOR_TYPE_FRAME_SENDER;
#endif  // USE_RPC
  } else if (type == "FramePublisher") {
    return OPERATOR_TYPE_FRAME_PUBLISHER;
  } else if (type == "FrameSubscriber") {
    return OPERATOR_TYPE_FRAME_SUBSCRIBER;
  } else if (type == "FrameWriter") {
    return OPERATOR_TYPE_FRAME_WRITER;
  } else if (type == "ImageClassifier") {
    return OPERATOR_TYPE_IMAGE_CLASSIFIER;
  } else if (type == "ImageSegmenter") {
    return OPERATOR_TYPE_IMAGE_SEGMENTER;
  } else if (type == "ImageTransformer") {
    return OPERATOR_TYPE_IMAGE_TRANSFORMER;
  } else if (type == "JpegWriter") {
    return OPERATOR_TYPE_JPEG_WRITER;
  } else if (type == "NeuralNetEvaluator") {
    return OPERATOR_TYPE_NEURAL_NET_EVALUATOR;
  } else if (type == "ObjectDetector") {
    return OPERATOR_TYPE_OBJECT_DETECTOR;
  } else if (type == "ObjectTracker") {
    return OPERATOR_TYPE_OBJECT_TRACKER;
  } else if (type == "ObjectMatcher") {
    return OPERATOR_TYPE_OBJECT_MATCHER;
  } else if (type == "OpenCVMotionDetector") {
    return OPERATOR_TYPE_OPENCV_MOTION_DETECTOR;
  } else if (type == "Strider") {
    return OPERATOR_TYPE_STRIDER;
  } else if (type == "TemporalRegionSelector") {
    return OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR;
  } else if (type == "Throttler") {
    return OPERATOR_TYPE_THROTTLER;
  } else if (type == "Sender") {
    return OPERATOR_TYPE_SENDER;
  } else if (type == "Receiver") {
    return OPERATOR_TYPE_RECEIVER;
  } else if (type == "FeatureExtractor") {
    return OPERATOR_TYPE_FEATURE_EXTRACTOR;
  } else {
    return OPERATOR_TYPE_INVALID;
  }
}

// Returns a human-readable string corresponding to the provided OperatorType.
inline std::string GetStringForOperatorType(OperatorType type) {
  switch (type) {
    case OPERATOR_TYPE_BINARY_FILE_WRITER:
      return "BinaryFileWriter";
    case OPERATOR_TYPE_BUFFER:
      return "Buffer";
    case OPERATOR_TYPE_CAMERA:
      return "Camera";
    case OPERATOR_TYPE_COMPRESSOR:
      return "Compressor";
    case OPERATOR_TYPE_CUSTOM:
      return "Custom";
    case OPERATOR_TYPE_WRITER:
      return "Writer";
    case OPERATOR_TYPE_DISPLAY:
      return "Display";
    case OPERATOR_TYPE_ENCODER:
      return "GstVideoEncoder";
    case OPERATOR_TYPE_FACE_TRACKER:
      return "FaceTracker";
#ifdef USE_CAFFE
    case OPERATOR_TYPE_FACENET:
      return "Facenet";
#endif  // USE_CAFFE
    case OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE:
      return "FlowControlEntrance";
    case OPERATOR_TYPE_FLOW_CONTROL_EXIT:
      return "FlowControlExit";
#ifdef USE_RPC
    case OPERATOR_TYPE_FRAME_RECEIVER:
      return "FrameReceiver";
    case OPERATOR_TYPE_FRAME_SENDER:
      return "FrameSender";
#endif  // USE_RPC
    case OPERATOR_TYPE_FRAME_PUBLISHER:
      return "FramePublisher";
    case OPERATOR_TYPE_FRAME_SUBSCRIBER:
      return "FrameSubscriber";
    case OPERATOR_TYPE_FRAME_WRITER:
      return "FrameWriter";
    case OPERATOR_TYPE_IMAGE_CLASSIFIER:
      return "ImageClassifier";
    case OPERATOR_TYPE_IMAGE_SEGMENTER:
      return "ImageSegmenter";
    case OPERATOR_TYPE_IMAGE_TRANSFORMER:
      return "ImageTransformer";
    case OPERATOR_TYPE_JPEG_WRITER:
      return "JpegWriter";
    case OPERATOR_TYPE_NEURAL_NET_EVALUATOR:
      return "NeuralNetEvaluator";
    case OPERATOR_TYPE_OBJECT_DETECTOR:
      return "ObjectDetector";
    case OPERATOR_TYPE_OBJECT_TRACKER:
      return "ObjectTracker";
    case OPERATOR_TYPE_OBJECT_MATCHER:
      return "ObjectMatcher";
    case OPERATOR_TYPE_OPENCV_MOTION_DETECTOR:
      return "OpenCVMotionDetector";
    case OPERATOR_TYPE_OPENCV_OPTICAL_FLOW:
      return "OpenCVOpticalFlow";
    case OPERATOR_TYPE_STRIDER:
      return "Strider";
    case OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR:
      return "TemporalRegionSelector";
    case OPERATOR_TYPE_THROTTLER:
      return "Throttler";
    case OPERATOR_TYPE_SENDER:
      return "Sender";
    case OPERATOR_TYPE_RECEIVER:
      return "Receiver";
    case OPERATOR_TYPE_FEATURE_EXTRACTOR:
      return "FeatureExtractor";
    case OPERATOR_TYPE_INVALID:
      return "Invalid";
  }

  LOG(FATAL) << "Unhandled OperatorType: " << type;
  return "Unhandled";
}

#endif  // SAF_COMMON_TYPES_H_
