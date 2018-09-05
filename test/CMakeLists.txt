# Copyright 2018 The SAF Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Set up include directories.
include_directories(SYSTEM ${gtest_SOURCE_DIR} ${gtest_SOURCE_DIR}/include)
include_directories(SYSTEM ${gmock_SOURCE_DIR} ${gmock_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/test/include)

# Pick source files.
file(GLOB_RECURSE TEST_SRCS ${PROJECT_SOURCE_DIR}/test/test_*.cpp)

if (USE_RPC)
  include_directories(SYSTEM ${CMAKE_BINARY_DIR}/src)
endif ()

if (USE_CAFFE)
  include_directories(SYSTEM ${Caffe_INCLUDE_DIRS})
else ()
  list(REMOVE_ITEM TEST_SRCS ${PROJECT_SOURCE_DIR}/test/test_nne_caffe.cpp)
endif ()

add_library(saf_gtest_main saf_gtest_main.cpp)
target_link_libraries(saf_gtest_main saf gtest)
add_build_reqs(saf_gtest_main)

add_custom_target(tests)
set(TEST_CONFIG_DIR ${PROJECT_BINARY_DIR}/test/config)
foreach (f ${TEST_SRCS})
  get_filename_component(basename ${f} NAME_WE)
  add_executable(${basename} EXCLUDE_FROM_ALL ${f})
  target_link_libraries(${basename} saf gtest saf_gtest_main gmock gmock_main)
  add_test(${basename} ${basename} ${TEST_CONFIG_DIR})
  add_dependencies(tests ${basename})
  add_build_reqs(${basename})
endforeach ()

# Set up test config and data files.
file(GLOB TEST_CONFIG_FILES ${PROJECT_SOURCE_DIR}/test/config/*.toml)
file(GLOB TEST_DATA_FILES ${PROJECT_SOURCE_DIR}/test/data/*)
foreach (f ${TEST_CONFIG_FILES})
  file(COPY ${f} DESTINATION config)
endforeach ()
foreach (f ${TEST_DATA_FILES})
  file(COPY ${f} DESTINATION data)
endforeach ()
