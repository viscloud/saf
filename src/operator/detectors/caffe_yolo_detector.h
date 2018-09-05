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

// Multi-target detection using YOLO

#ifndef SAF_OPERATOR_DETECTORS_CAFFE_YOLO_DETECTOR_H_
#define SAF_OPERATOR_DETECTORS_CAFFE_YOLO_DETECTOR_H_

#include <set>

#include <caffe/caffe.hpp>

#include "model/model.h"
#include "operator/detectors/object_detector.h"

namespace yolo {
class Detector {
 public:
  Detector(const std::string& model_file, const std::string& weights_file);

  std::vector<float> Detect(const cv::Mat& img);

 private:
  std::shared_ptr<caffe::Net<float>> net_;
  cv::Size input_geometry_;
  int num_channels_;
  cv::Mat mean_;
};
}  // namespace yolo

class YoloDetector : public BaseDetector {
 public:
  YoloDetector(const ModelDesc& model_desc) : model_desc_(model_desc) {}
  virtual ~YoloDetector() {}
  virtual bool Init();
  virtual std::vector<ObjectInfo> Detect(const cv::Mat& image);

 private:
  ModelDesc model_desc_;
  std::unique_ptr<yolo::Detector> detector_;
  std::vector<std::string> voc_names_;
};

template <typename Dtype>
Dtype lap(Dtype x1_min, Dtype x1_max, Dtype x2_min, Dtype x2_max) {
  if (x1_min < x2_min) {
    if (x1_max < x2_min) {
      return 0;
    } else {
      if (x1_max > x2_min) {
        if (x1_max < x2_max) {
          return x1_max - x2_min;
        } else {
          return x2_max - x2_min;
        }
      } else {
        return 0;
      }
    }
  } else {
    if (x1_min < x2_max) {
      if (x1_max < x2_max)
        return x1_max - x1_min;
      else
        return x2_max - x1_min;
    } else {
      return 0;
    }
  }
}

template int lap(int x1_min, int x1_max, int x2_min, int x2_max);
template float lap(float x1_min, float x1_max, float x2_min, float x2_max);

inline std::vector<std::vector<int>> GetBoxes(
    std::vector<float> DetectionResult, float* pro_obj, int* idx_class,
    std::vector<std::vector<int>>& bboxs, float thresh, cv::Mat img) {
  float overlap;
  float overlap_thresh = 0.4;
  float pro_class[49];
  int idx;
  int idx2;
  float max_idx;
  float max;

  for (int i = 0; i < 7; ++i) {
    for (int j = 0; j < 7; ++j) {
      max = 0;
      max_idx = 0;
      idx2 = 20 * (i * 7 + j);
      for (int k = 0; k < 20; ++k) {
        if (DetectionResult[idx2 + k] > max) {
          max = DetectionResult[idx2 + k];
          max_idx = k + 1;
        }
      }
      idx_class[i * 7 + j] = max_idx;
      pro_class[i * 7 + j] = max;
      pro_obj[(i * 7 + j) * 2] =
          max * DetectionResult[7 * 7 * 20 + (i * 7 + j) * 2];
      pro_obj[(i * 7 + j) * 2 + 1] =
          max * DetectionResult[7 * 7 * 20 + (i * 7 + j) * 2 + 1];
    }
  }

  std::vector<int> bbox;
  int x_min, x_max, y_min, y_max;
  float x, y, w, h;
  for (int i = 0; i < 7; ++i) {
    for (int j = 0; j < 7; ++j) {
      for (int k = 0; k < 2; ++k) {
        if (pro_obj[(i * 7 + j) * 2 + k] > thresh) {
          idx = 49 * 20 + 49 * 2 + ((i * 7 + j) * 2 + k) * 4;
          x = img.cols * (DetectionResult[idx++] + j) / 7;
          y = img.rows * (DetectionResult[idx++] + i) / 7;
          w = img.cols * DetectionResult[idx] * DetectionResult[idx];
          ++idx;
          h = img.rows * DetectionResult[idx] * DetectionResult[idx];
          x_min = x - w / 2;
          y_min = y - h / 2;
          x_max = x + w / 2;
          y_max = y + h / 2;
          bbox.clear();
          bbox.push_back(idx_class[i * 7 + j]);
          bbox.push_back(x_min);
          bbox.push_back(y_min);
          bbox.push_back(x_max);
          bbox.push_back(y_max);
          bbox.push_back(int(pro_obj[(i * 7 + j) * 2 + k] * 100));
          bboxs.push_back(bbox);
        }
      }
    }
  }

  std::vector<bool> mark(bboxs.size(), true);
  for (size_t i = 0; i < bboxs.size(); ++i) {
    for (size_t j = i + 1; j < bboxs.size(); ++j) {
      int overlap_x = lap(bboxs[i][0], bboxs[i][2], bboxs[j][0], bboxs[j][2]);
      int overlap_y = lap(bboxs[i][1], bboxs[i][3], bboxs[j][1], bboxs[j][3]);
      overlap = (overlap_x * overlap_y) * 1.0 /
                ((bboxs[i][0] - bboxs[i][2]) * (bboxs[i][1] - bboxs[i][3]) +
                 (bboxs[j][0] - bboxs[j][2]) * (bboxs[j][1] - bboxs[j][3]) -
                 (overlap_x * overlap_y));
      if (overlap > overlap_thresh) {
        if (bboxs[i][4] > bboxs[j][4]) {
          mark[j] = false;
        } else {
          mark[i] = false;
        }
      }
    }
  }

  std::vector<std::vector<int>> Boxes;
  for (size_t i = 0; i < bboxs.size(); ++i) {
    const std::vector<int>& d = bboxs[i];
    if (mark[i]) {
      Boxes.push_back(d);
    }
  }

  (void)pro_class;
  return Boxes;
}

#endif  // SAF_OPERATOR_DETECTORS_CAFFE_YOLO_DETECTOR_H_
