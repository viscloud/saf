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

// Multi-target matching using Euclidean distance

#ifndef SAF_OPERATOR_MATCHERS_EUCLIDEAN_MATCHER_H_
#define SAF_OPERATOR_MATCHERS_EUCLIDEAN_MATCHER_H_

#include <cmath>

#include "common/context.h"
#include "model/model.h"
#include "operator/matchers/object_matcher.h"

class EuclideanMatcher : public BaseMatcher {
 public:
  EuclideanMatcher() {}
  virtual ~EuclideanMatcher() {}
  virtual bool Init() { return true; };
  virtual double Match(const std::vector<double>& feat1,
                       const std::vector<double>& feat2) {
    double sum, sum1, sum2;
    sum = sum1 = sum2 = 0;
    CHECK(feat1.size() == feat2.size());

    // Normalize feat1 & feat2
    for (size_t i = 0; i < feat1.size(); i++) {
      sum1 = sum1 + std::pow(feat1[i], 2);
      sum2 = sum2 + std::pow(feat2[i], 2);
    }

    sum1 = std::sqrt(sum1);
    sum2 = std::sqrt(sum2);

    // Calculate Eeuclidean distance
    for (size_t i = 0; i < feat1.size(); i++) {
      sum = sum + std::pow(feat1[i] / sum1 - feat2[i] / sum2, 2);
    }
    sum = std::sqrt(sum);
    return sum;
  }

 private:
};

#endif  // SAF_OPERATOR_MATCHERS_EUCLIDEAN_MATCHER_H_
