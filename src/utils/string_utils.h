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

#ifndef SAF_UTILS_STRING_UTILS_H_
#define SAF_UTILS_STRING_UTILS_H_

#include <glog/logging.h>
#include <regex>
#include <sstream>

#include <boost/algorithm/string.hpp>

/**
 * @brief Determine if a string ends with certain suffix.
 *
 * @param str The string to check.
 * @param ending The suffix.
 *
 * @return True if the string ends with <code>ending</code>.
 */
inline bool EndsWith(const std::string& str, const std::string& ending) {
  if (ending.size() > str.size()) return false;
  return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

/**
 * @brief Determine if a string starts with certain prefix.
 *
 * @param str The string to check.
 * @param ending The prefix.
 *
 * @return True if the string starts with <code>start</code>.
 */
inline bool StartsWith(const std::string& str, const std::string& start) {
  if (start.size() > str.size()) return false;
  return std::equal(start.begin(), start.end(), str.begin());
}

inline std::string TrimSpaces(const std::string& str) {
  size_t first = str.find_first_not_of(' ');
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

inline std::vector<std::string> SplitString(const std::string& str,
                                            const std::string& delim) {
  std::vector<std::string> results;
  boost::split(results, str, boost::is_any_of(delim));
  return results;
}

/**
 * @brief Get protocol name and path from an uri
 * @param uri An uri in the form protocol://path
 * @param protocol The reference to store the protocol.
 * @param path The reference to store the path.
 */
inline void ParseProtocolAndPath(const std::string& uri, std::string& protocol,
                                 std::string& path) {
  std::regex re("(.+?)://(.+)");
  std::smatch results;
  if (!std::regex_match(uri, results, re)) {
    LOG(FATAL) << "Cannot parse URI: " << uri;
  }
  protocol = results[1];
  path = results[2];
}

/**
 * @brief Get a numeric value for ip address from a string. 1.2.3.4 will be
 * converted to 0x01020304.
 * @param ip_str The ip address in a string
 * @return The ip address in integer
 */
inline unsigned int GetIPAddrFromString(const std::string& ip_str) {
  std::vector<std::string> sp = SplitString(ip_str, ".");
  unsigned int ip_val = 0;
  CHECK(sp.size() == 4) << ip_str << " is not a valid ip address";

  for (int i = 0; i < 4; i++) {
    ip_val += atoi(sp[i].c_str()) << ((3 - i) * 8);
  }

  return ip_val;
}

/**
 * @brief Check if a string contains a substring.
 * @param str The string to be checked.
 * @param substr The substring to be checked.
 * @return Wether the string has the substring or not.
 */
inline bool StringContains(const std::string& str, const std::string& substr) {
  return str.find(substr) != std::string::npos;
}

/**
 * @brief Convert an string to an interger.
 * @param str The string to be converted.
 * @return The converted integer.
 */
inline int StringToInt(const std::string& str) { return atoi(str.c_str()); }

inline size_t StringToSizet(const std::string& str) {
  std::istringstream iss(str);
  size_t s;
  iss >> s;
  if (iss.fail()) {
    LOG(FATAL) << "Improperly formed size_t: " << str;
  }
  return s;
}

#endif  // SAF_UTILS_STRING_UTILS_H_
