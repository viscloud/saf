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

// Multi-target detection using MTCNN

#ifndef SAF_OPERATOR_DETECTORS_CAFFE_MTCNN_FACE_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_CAFFE_MTCNN_FACE_DETECTOR_H_

#include <fstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <caffe/caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <opencv2/opencv.hpp>

#include "model/model.h"
#include "operator/detectors/object_detector.h"

using caffe::Blob;
using caffe::Caffe;
using caffe::Datum;
using caffe::MemoryDataLayer;
using caffe::Net;
using caffe::TEST;

struct FaceRect {
  float x1;
  float y1;
  float x2;
  float y2;
  float score; /**< Larger score should mean higher confidence. */
};

struct FacePts {
  float x[5], y[5];
};

struct FaceInfo {
  FaceRect bbox;
  cv::Vec4f regression;
  FacePts facePts;
  double roll;
  double pitch;
  double yaw;
};

class MTCNN {
 public:
  MTCNN(const std::vector<ModelDesc>& model_descs);
  void Detect(const cv::Mat& img, std::vector<FaceInfo>& faceInfo, int minSize,
              double* threshold, double factor);

 private:
  bool CvMatToDatumSignalChannel(const cv::Mat& cv_mat, Datum* datum);
  void Preprocess(const cv::Mat& img, std::vector<cv::Mat>* input_channels);
  void WrapInputLayer(std::vector<cv::Mat>* input_channels,
                      Blob<float>* input_layer, const int height,
                      const int width);
  void SetMean();
  void GenerateBoundingBox(Blob<float>* confidence, Blob<float>* reg,
                           float scale, float thresh, int image_width,
                           int image_height);
  void ClassifyFace(const std::vector<FaceInfo>& regressed_rects,
                    cv::Mat& sample_single, boost::shared_ptr<Net<float> >& net,
                    double thresh, char netName);
  void ClassifyFace_MulImage(const std::vector<FaceInfo>& regressed_rects,
                             cv::Mat& sample_single,
                             boost::shared_ptr<Net<float> >& net, double thresh,
                             char netName);
  std::vector<FaceInfo> NonMaximumSuppression(std::vector<FaceInfo>& bboxes,
                                              float thresh, char methodType);
  void Bbox2Square(std::vector<FaceInfo>& bboxes);
  void Padding(int img_w, int img_h);
  std::vector<FaceInfo> BoxRegress(std::vector<FaceInfo>& faceInfo_, int stage);
  void RegressPoint(const std::vector<FaceInfo>& faceInfo);

 private:
  boost::shared_ptr<Net<float> > PNet_;
  boost::shared_ptr<Net<float> > RNet_;
  boost::shared_ptr<Net<float> > ONet_;

  // x1,y1,x2,t2 and score
  std::vector<FaceInfo> condidate_rects_;
  std::vector<FaceInfo> total_boxes_;
  std::vector<FaceInfo> regressed_rects_;
  std::vector<FaceInfo> regressed_pading_;

  std::vector<cv::Mat> crop_img_;
  int curr_feature_map_w_;
  int curr_feature_map_h_;
  int num_channels_;
};

class MtcnnFaceDetector : public BaseDetector {
 public:
  MtcnnFaceDetector(const std::vector<ModelDesc>& model_descs,
                    int min_size /* default is 40 */)
      : model_descs_(model_descs), minSize_(min_size) {
    threshold_[0] = 0.6;
    threshold_[1] = 0.7;
    threshold_[2] = 0.7;
    factor_ = 0.709;
  }
  virtual ~MtcnnFaceDetector() {}
  virtual bool Init();
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image);

 private:
  std::vector<ModelDesc> model_descs_;
  std::unique_ptr<MTCNN> detector_;
  double threshold_[3];
  double factor_;
  int minSize_;
};

#endif  // SAF_OPERATOR_DETECTORS_CAFFE_MTCNN_FACE_DETECTOR_H_
