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

#ifndef SAF_SAF_H_
#define SAF_SAF_H_

#include "camera/camera.h"
#include "camera/camera_manager.h"
#include "camera/gst_camera.h"
#include "common/context.h"
#include "common/serialization.h"
#include "common/timer.h"
#include "common/types.h"
#include "model/model.h"
#include "model/model_manager.h"
#include "operator/binary_file_writer.h"
#include "operator/buffer.h"
#include "operator/compressor.h"
#include "operator/detectors/object_detector.h"
#include "operator/detectors/opencv_face_detector.h"
#include "operator/detectors/opencv_people_detector.h"
#include "operator/display.h"
#include "operator/extractors/feature_extractor.h"
#include "operator/face_tracker.h"
#include "operator/flow_control/flow_control_entrance.h"
#include "operator/flow_control/flow_control_exit.h"
#include "operator/frame_writer.h"
#include "operator/image_classifier.h"
#include "operator/image_segmenter.h"
#include "operator/image_transformer.h"
#include "operator/jpeg_writer.h"
#include "operator/matchers/euclidean_matcher.h"
#include "operator/matchers/object_matcher.h"
#include "operator/matchers/xqda_matcher.h"
#include "operator/neural_net_consumer.h"
#include "operator/neural_net_evaluator.h"
#include "operator/opencv_motion_detector.h"
#include "operator/opencv_optical_flow.h"
#include "operator/operator.h"
#include "operator/operator_factory.h"
#include "operator/pubsub/frame_publisher.h"
#include "operator/pubsub/frame_subscriber.h"
#include "operator/receivers/receiver.h"
#include "operator/rtsp_sender.h"
#include "operator/senders/sender.h"
#include "operator/strider.h"
#include "operator/temporal_region_selector.h"
#include "operator/throttler.h"
#include "operator/trackers/object_tracker.h"
#include "operator/writers/file_writer.h"
#include "operator/writers/writer.h"
#include "pipeline/pipeline.h"
#include "stream/frame.h"
#include "stream/stream.h"
#include "utils/cuda_utils.h"
#include "utils/cv_utils.h"
#include "utils/file_utils.h"
#include "utils/fp16.h"
#include "utils/gst_utils.h"
#include "utils/hash_utils.h"
#include "utils/image_utils.h"
#include "utils/math_utils.h"
#include "utils/output_tracker.h"
#include "utils/perf_utils.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"
#include "utils/utils.h"
#include "utils/yolo_utils.h"
#include "video/gst_video_capture.h"
#include "video/gst_video_encoder.h"

#ifdef USE_CAFFE
#include "model/caffe_model.h"
#include "operator/caffe_facenet.h"
#include "operator/detectors/caffe_mtcnn_face_detector.h"
#include "operator/detectors/caffe_yolo_detector.h"
#endif  // USE_CAFFE
#ifdef HAVE_INTEL_CAFFE
#include "operator/detectors/caffe_yolo_v2_detector.h"
#include "operator/extractors/caffe_feature_extractor.h"
#ifdef USE_SSD
#include "operator/detectors/caffe_mobilenet_ssd_detector.h"
#include "operator/detectors/ssd_detector.h"
#endif  // USE_SSD
#endif  // HAVE_INTEL_CAFFE

#ifdef USE_CVSDK
#include "model/cvsdk_model.h"
#include "operator/extractors/cvsdk_feature_extractor.h"
#ifdef USE_SSD
#include "operator/detectors/cvsdk_ssd_detector.h"
#endif  // USE_SSD
#endif  // USE_CVSDK

#ifdef USE_DLIB
#include "operator/trackers/dlib_tracker.h"
#endif  // USE_DLIB

#ifdef USE_FCRNN
#include "operator/detectors/frcnn_detector.h"
#endif  // USE_FRCNN

#ifdef USE_KAFKA
#include "operator/receivers/kafka_receiver.h"
#include "operator/senders/kafka_sender.h"
#endif

#ifdef USE_MQTT
#include "operator/receivers/mqtt_receiver.h"
#include "operator/senders/mqtt_sender.h"
#endif  // USE_MQTT

#ifdef USE_NCS
#include "ncs/ncs.h"
#include "ncs/ncs_manager.h"
#include "operator/detectors/ncs_yolo_detector.h"
#endif  // USE_NCS

#ifdef USE_PTGRAY
#include "camera/pgr_camera.h"
#endif  // USE_PTGRAY

#ifdef USE_RPC
#include "operator/rpc/frame_receiver.h"
#include "operator/rpc/frame_sender.h"
#endif  // USE_RPC

#ifdef USE_TENSORFLOW
#include "model/tf_model.h"
#endif  // USE_TENSORFLOW

#ifdef USE_VIMBA
#include "camera/vimba_camera.h"
#endif  // USE_VIMBA

#ifdef USE_WEBSOCKET
#include "operator/receivers/websocket_receiver.h"
#include "operator/senders/websocket_sender.h"
#endif  // USE_WEBSOCKET

#endif  // SAF_SAF_H_
