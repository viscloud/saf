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

#ifndef SAF_UTILS_YOLO_UTILS_H_
#define SAF_UTILS_YOLO_UTILS_H_
#include <fstream>
#include <opencv2/opencv.hpp>

typedef struct {
  int index;
  int nclass;
  std::vector<std::vector<float>> probs;  // turn into pointer
} sortable_bbox;

inline static int nms_comparator(const void* pa, const void* pb) {
  sortable_bbox a = *(sortable_bbox*)pa;
  sortable_bbox b = *(sortable_bbox*)pb;
  float diff = a.probs[a.index][b.nclass] - b.probs[b.index][b.nclass];
  if (diff < 0)
    return 1;
  else if (diff > 0)
    return -1;
  return 0;
}

inline static float overlap(float x1, float w1, float x2, float w2) {
  float l1 = x1 - w1 / 2;
  float l2 = x2 - w2 / 2;
  float left = l1 > l2 ? l1 : l2;
  float r1 = x1 + w1 / 2;
  float r2 = x2 + w2 / 2;
  float right = r1 < r2 ? r1 : r2;
  return right - left;
}

inline static float box_intersection(const cv::Rect_<float>& a,
                                     const cv::Rect_<float>& b) {
  float w = overlap(a.x, a.width, b.x, b.width);
  float h = overlap(a.y, a.height, b.y, b.height);
  if (w < 0 || h < 0) return 0;
  float area = w * h;
  return area;
}

inline static float box_union(const cv::Rect_<float>& a,
                              const cv::Rect_<float>& b) {
  float i = box_intersection(a, b);
  float u = a.width * a.height + b.width * b.height - i;
  return u;
}

inline static float box_iou(const cv::Rect_<float>& a,
                            const cv::Rect_<float>& b) {
  return box_intersection(a, b) / box_union(a, b);
}

inline static int max_index(const float* a, int n) {
  if (n <= 0) return -1;
  int i, max_i = 0;
  float max = a[0];
  for (i = 1; i < n; ++i) {
    if (a[i] > max) {
      max = a[i];
      max_i = i;
    }
  }
  return max_i;
}

inline static void get_boxes(std::vector<std::vector<float>>& probs,
                             std::vector<cv::Rect_<float>>& boxes,
                             const std::vector<float>& predictions, int classes,
                             bool only_objectness = false) {
  float thresh = 0.1f;

  // The following are for YOLOv1-tiny
  assert(predictions.size() == 1470);
  const int side = 7;
  const int num = 2;
  const int sqrt = 1;
  const int w = 1;
  const int h = 1;
  // The above are for YOLOv1-tiny

  boxes.resize(side * side * num);
  probs.resize(side * side * num, std::vector<float>(classes));
  for (int i = 0; i < side * side; ++i) {
    int row = i / side;
    int col = i % side;
    for (int n = 0; n < num; ++n) {
      int index = i * num + n;
      int p_index = side * side * classes + i * num + n;
      float scale = predictions[p_index];
      int box_index = side * side * (classes + num) + (i * num + n) * 4;
      boxes[index].x = (predictions[box_index + 0] + col) / side * w;
      boxes[index].y = (predictions[box_index + 1] + row) / side * h;
      boxes[index].width = pow(predictions[box_index + 2], (sqrt ? 2 : 1)) * w;
      boxes[index].height = pow(predictions[box_index + 3], (sqrt ? 2 : 1)) * h;
      for (int j = 0; j < classes; ++j) {
        int class_index = i * classes;
        float prob = scale * predictions[class_index + j];
        probs[index][j] = (prob > thresh) ? prob : 0;
      }
      if (only_objectness) {
        probs[index][0] = scale;
      }
    }
  }
}

inline static void nms_sort(std::vector<std::vector<float>>& probs,
                            const std::vector<cv::Rect_<float>>& boxes,
                            int classes) {
  float thresh = 0.35f;

  int total = boxes.size();
  sortable_bbox* s = (sortable_bbox*)calloc(total, sizeof(sortable_bbox));

  for (int i = 0; i < total; ++i) {
    s[i].index = i;
    s[i].nclass = 0;
    s[i].probs = probs;
  }

  for (int k = 0; k < classes; ++k) {
    for (int i = 0; i < total; ++i) {
      s[i].nclass = k;
    }
    qsort(s, total, sizeof(sortable_bbox), nms_comparator);
    for (int i = 0; i < total; ++i) {
      if (probs[s[i].index][k] == 0) continue;
      const cv::Rect_<float>& a = boxes[s[i].index];
      for (int j = i + 1; j < total; ++j) {
        const cv::Rect_<float>& b = boxes[s[j].index];
        if (box_iou(a, b) > thresh) {
          probs[s[j].index][k] = 0;
        }
      }
    }
  }
  free(s);
}

inline static void get_detections(
    std::vector<std::tuple<int, cv::Rect, float>>& detections,
    const cv::Size& size, const std::vector<std::vector<float>>& probs,
    const std::vector<cv::Rect_<float>>& boxes, int classes) {
  float thresh = 0.1f;

  int num = boxes.size();

  for (int i = 0; i < num; ++i) {
    int j = max_index(&probs[i][0], classes);
    float prob = probs[i][j];
    if (prob > thresh) {
      const cv::Rect_<float>& b = boxes[i];

      int left = (b.x - b.width / 2.) * size.width;
      int right = (b.x + b.width / 2.) * size.width;
      int top = (b.y - b.height / 2.) * size.height;
      int bot = (b.y + b.height / 2.) * size.height;

      if (left < 0) left = 0;
      if (right > size.width - 1) right = size.width - 1;
      if (top < 0) top = 0;
      if (bot > size.height - 1) bot = size.height - 1;

      detections.push_back(std::make_tuple(
          j, cv::Rect(left, top, right - left, bot - top), prob));
    }
  }
}

inline static void get_detections(
    std::vector<std::tuple<int, cv::Rect, float>>& detections,
    const std::vector<float>& predictions, const cv::Size& size, int classes) {
  std::vector<std::vector<float>> probs;
  std::vector<cv::Rect_<float>> boxes;

  get_boxes(probs, boxes, predictions, classes);
  nms_sort(probs, boxes, classes);
  get_detections(detections, size, probs, boxes, classes);
}

inline static void draw_detections(
    cv::Mat& image,
    const std::vector<std::tuple<int, cv::Rect, float>>& detections,
    const std::vector<std::string>& classes) {
  for (size_t i = 0; i < detections.size(); i++) {
    cv::rectangle(image, std::get<1>(detections[i]), cv::Scalar(255, 0, 0), 3);
    cv::putText(image,
                classes[std::get<0>(detections[i])] + " - " +
                    std::to_string((int)(std::get<2>(detections[i]) * 100)) +
                    "%",
                std::get<1>(detections[i]).tl(), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                cv::Scalar(0, 0, 255), 2.0);
  }
}

inline static std::vector<std::string> ReadVocNames(
    const std::string& file_path) {
  std::vector<std::string> result;
  std::string name;
  std::ifstream infile;
  infile.open(file_path.c_str());
  CHECK(infile) << "Cannot open " << file_path;
  result.push_back("none_of_the_above");
  while (std::getline(infile, name)) {
    result.push_back(name);
  }
  infile.close();
  return result;
}

#endif  // SAF_UTILS_YOLO_UTILS_H_
