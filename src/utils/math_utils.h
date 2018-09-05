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

#ifndef SAF_UTILS_MATH_UTILS_H_
#define SAF_UTILS_MATH_UTILS_H_

inline bool PairCompare(const std::pair<float, int>& lhs,
                        const std::pair<float, int>& rhs) {
  return lhs.first > rhs.first;
}

/**
 * @brief Find the largest K numbers for <code>scores</code>.
 * @param scores The list of scores to be compared.
 * @param N The length of scores.
 * @param K Number of numbers to be selected
 * @return
 */
inline std::vector<int> Argmax(float* scores, int N, int K) {
  std::vector<std::pair<float, int>> pairs;
  for (decltype(N) i = 0; i < N; ++i)
    pairs.push_back(std::make_pair(scores[i], i));
  std::partial_sort(pairs.begin(), pairs.begin() + K, pairs.end(), PairCompare);

  std::vector<int> result;
  for (decltype(K) i = 0; i < K; ++i) result.push_back(pairs[i].second);
  return result;
}

#endif  // SAF_UTILS_MATH_UTILS_H_
