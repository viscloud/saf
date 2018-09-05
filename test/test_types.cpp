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

#include <gtest/gtest.h>
#include <json/src/json.hpp>

#include "common/types.h"

// Verifies that Rect::ToJson() produces a correctly-formatted JSON object The
// resulting JSON should look like this:
//   {
//     "Rect": {
//       "px": 1,
//       "py": 2,
//       "width": 3,
//       "height": 4
//     }
//   }
TEST(TestTypes, TestRectToJson) {
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;

  Rect r(a, b, c, d);
  nlohmann::json j = r.ToJson();
  nlohmann::json rect_j = j.at("Rect");

  EXPECT_EQ(rect_j.at("px").get<int>(), a);
  EXPECT_EQ(rect_j.at("py").get<int>(), b);
  EXPECT_EQ(rect_j.at("width").get<int>(), c);
  EXPECT_EQ(rect_j.at("height").get<int>(), d);
}

// Verifies that Rect::Rect(nlohmann::json) creates a properly-initialized Rect
// struct from a JSON object. See TestRectToJson for details on the format of
// the JSON object.
TEST(TestTypes, TestJsonToRect) {
  int a = 1;
  int b = 2;
  int c = 3;
  int d = 4;

  nlohmann::json rect_j;
  rect_j["px"] = a;
  rect_j["py"] = b;
  rect_j["width"] = c;
  rect_j["height"] = d;
  nlohmann::json j;
  j["Rect"] = rect_j;
  Rect r(j);

  EXPECT_EQ(r.px, a);
  EXPECT_EQ(r.py, b);
  EXPECT_EQ(r.width, c);
  EXPECT_EQ(r.height, d);
}

TEST(TestTypes, TestOperatorTypesStringConversion) {
  EXPECT_EQ(OPERATOR_TYPE_BINARY_FILE_WRITER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_BINARY_FILE_WRITER)));
  EXPECT_EQ(
      OPERATOR_TYPE_BUFFER,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_BUFFER)));
  EXPECT_EQ(
      OPERATOR_TYPE_CAMERA,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_CAMERA)));
  EXPECT_EQ(OPERATOR_TYPE_COMPRESSOR,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_COMPRESSOR)));
  EXPECT_EQ(
      OPERATOR_TYPE_CUSTOM,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_CUSTOM)));
  EXPECT_EQ(
      OPERATOR_TYPE_DISPLAY,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_DISPLAY)));
  EXPECT_EQ(
      OPERATOR_TYPE_ENCODER,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_ENCODER)));
  EXPECT_EQ(OPERATOR_TYPE_FACE_TRACKER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FACE_TRACKER)));
#ifdef USE_CAFFE
  EXPECT_EQ(
      OPERATOR_TYPE_FACENET,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_FACENET)));
#endif  // USE_CAFFE
  EXPECT_EQ(OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FLOW_CONTROL_ENTRANCE)));
  EXPECT_EQ(OPERATOR_TYPE_FLOW_CONTROL_EXIT,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FLOW_CONTROL_EXIT)));
#ifdef USE_RPC
  EXPECT_EQ(OPERATOR_TYPE_FRAME_RECEIVER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FRAME_RECEIVER)));
  EXPECT_EQ(OPERATOR_TYPE_FRAME_SENDER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FRAME_SENDER)));
#endif  // USE_RPC
  EXPECT_EQ(OPERATOR_TYPE_FRAME_PUBLISHER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FRAME_PUBLISHER)));
  EXPECT_EQ(OPERATOR_TYPE_FRAME_SUBSCRIBER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FRAME_SUBSCRIBER)));
  EXPECT_EQ(OPERATOR_TYPE_FRAME_WRITER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_FRAME_WRITER)));
  EXPECT_EQ(OPERATOR_TYPE_IMAGE_CLASSIFIER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_IMAGE_CLASSIFIER)));
  EXPECT_EQ(OPERATOR_TYPE_IMAGE_SEGMENTER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_IMAGE_SEGMENTER)));
  EXPECT_EQ(OPERATOR_TYPE_IMAGE_TRANSFORMER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_IMAGE_TRANSFORMER)));
  EXPECT_EQ(OPERATOR_TYPE_JPEG_WRITER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_JPEG_WRITER)));
  EXPECT_EQ(OPERATOR_TYPE_NEURAL_NET_EVALUATOR,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_NEURAL_NET_EVALUATOR)));
  EXPECT_EQ(OPERATOR_TYPE_OBJECT_DETECTOR,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_OBJECT_DETECTOR)));
  EXPECT_EQ(OPERATOR_TYPE_OBJECT_TRACKER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_OBJECT_TRACKER)));
  EXPECT_EQ(OPERATOR_TYPE_OPENCV_MOTION_DETECTOR,
            GetOperatorTypeByString(GetStringForOperatorType(
                OPERATOR_TYPE_OPENCV_MOTION_DETECTOR)));
  EXPECT_EQ(
      OPERATOR_TYPE_STRIDER,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_STRIDER)));
  EXPECT_EQ(OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR,
            GetOperatorTypeByString(GetStringForOperatorType(
                OPERATOR_TYPE_TEMPORAL_REGION_SELECTOR)));
  EXPECT_EQ(OPERATOR_TYPE_THROTTLER,
            GetOperatorTypeByString(
                GetStringForOperatorType(OPERATOR_TYPE_THROTTLER)));
  EXPECT_EQ(
      OPERATOR_TYPE_INVALID,
      GetOperatorTypeByString(GetStringForOperatorType(OPERATOR_TYPE_INVALID)));
}
