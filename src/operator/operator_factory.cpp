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

#include "operator/operator_factory.h"

#include "camera/camera_manager.h"
#include "operator/binary_file_writer.h"
#include "operator/buffer.h"
#ifdef USE_CAFFE
#include "operator/caffe_facenet.h"
#include "operator/extractors/caffe_feature_extractor.h"
#endif  // USE_CAFFE
#include "operator/compressor.h"
#include "operator/detectors/object_detector.h"
#include "operator/display.h"
#include "operator/face_tracker.h"
#include "operator/flow_control/flow_control_entrance.h"
#include "operator/flow_control/flow_control_exit.h"
#include "operator/frame_writer.h"
#include "operator/image_classifier.h"
#include "operator/image_segmenter.h"
#include "operator/image_transformer.h"
#include "operator/jpeg_writer.h"
#include "operator/matchers/object_matcher.h"
#include "operator/neural_net_evaluator.h"
#include "operator/opencv_motion_detector.h"
#include "operator/opencv_optical_flow.h"
#include "operator/pubsub/frame_publisher.h"
#include "operator/pubsub/frame_subscriber.h"
#include "operator/trackers/object_tracker.h"
#ifdef USE_RPC
#include "operator/rpc/frame_receiver.h"
#include "operator/rpc/frame_sender.h"
#endif  // USE_RPC
#include "operator/extractors/feature_extractor.h"
#include "operator/receivers/receiver.h"
#include "operator/senders/sender.h"
#include "operator/strider.h"
#include "operator/temporal_region_selector.h"
#include "operator/throttler.h"
#include "operator/writers/writer.h"
#include "video/gst_video_encoder.h"

std::shared_ptr<Operator> OperatorFactory::Create(OperatorType type,
                                                  FactoryParamsType params) {
  switch (type) {
    case OPERATOR_TYPE_BINARY_FILE_WRITER:
      return BinaryFileWriter::Create(params);
    case OPERATOR_TYPE_BUFFER:
      return Buffer::Create(params);
    case OPERATOR_TYPE_CAMERA:
      return CameraManager::GetInstance().GetCamera(params.at("camera_name"));
    case OPERATOR_TYPE_COMPRESSOR:
      return Compressor::Create(params);
    case OPERATOR_TYPE_CUSTOM:
      SAF_NOT_IMPLEMENTED;
      return nullptr;
    case OPERATOR_TYPE_WRITER:
      return Writer::Create(params);
    case OPERATOR_TYPE_DISPLAY:
      return Display::Create(params);
    case OPERATOR_TYPE_ENCODER:
      return GstVideoEncoder::Create(params);
#ifdef USE_CAFFE
    case OPERATOR_TYPE_FACENET:
      return Facenet::Create(params);
#endif  // USE_CAFFE
    case OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE:
      return FlowControlEntrance::Create(params);
    case OPERATOR_TYPE_FLOW_CONTROL_EXIT:
      return FlowControlExit::Create(params);
#ifdef USE_RPC
    case OPERATOR_TYPE_FRAME_RECEIVER:
      return FrameReceiver::Create(params);
    case OPERATOR_TYPE_FRAME_SENDER:
      return FrameSender::Create(params);
#endif  // USE_RPC
    case OPERATOR_TYPE_FRAME_PUBLISHER:
      return FramePublisher::Create(params);
    case OPERATOR_TYPE_FRAME_SUBSCRIBER:
      return FrameSubscriber::Create(params);
    case OPERATOR_TYPE_FRAME_WRITER:
      return FrameWriter::Create(params);
    case OPERATOR_TYPE_IMAGE_CLASSIFIER:
      return ImageClassifier::Create(params);
    case OPERATOR_TYPE_IMAGE_SEGMENTER:
      return ImageSegmenter::Create(params);
    case OPERATOR_TYPE_IMAGE_TRANSFORMER:
      return ImageTransformer::Create(params);
    case OPERATOR_TYPE_JPEG_WRITER:
      return JpegWriter::Create(params);
    case OPERATOR_TYPE_NEURAL_NET_EVALUATOR:
      return NeuralNetEvaluator::Create(params);
    case OPERATOR_TYPE_OBJECT_MATCHER:
      return ObjectMatcher::Create(params);
    case OPERATOR_TYPE_OBJECT_TRACKER:
      return ObjectTracker::Create(params);
    case OPERATOR_TYPE_OBJECT_DETECTOR:
      return ObjectDetector::Create(params);
    case OPERATOR_TYPE_FACE_TRACKER:
      return FaceTracker::Create(params);
    case OPERATOR_TYPE_OPENCV_MOTION_DETECTOR:
      return OpenCVMotionDetector::Create(params);
    case OPERATOR_TYPE_OPENCV_OPTICAL_FLOW:
      return OpenCVOpticalFlow::Create(params);
    case OPERATOR_TYPE_STRIDER:
      return Strider::Create(params);
    case OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR:
      return TemporalRegionSelector::Create(params);
    case OPERATOR_TYPE_THROTTLER:
      return Throttler::Create(params);
    case OPERATOR_TYPE_SENDER:
      return Sender::Create(params);
    case OPERATOR_TYPE_RECEIVER:
      return Receiver::Create(params);
    case OPERATOR_TYPE_FEATURE_EXTRACTOR:
      return FeatureExtractor::Create(params);
    case OPERATOR_TYPE_INVALID:
      LOG(FATAL) << "Cannot instantiate a Operator of type: "
                 << GetStringForOperatorType(type);
  }

  LOG(FATAL) << "Unhandled OperatorType: " << GetStringForOperatorType(type);
  return nullptr;
}
