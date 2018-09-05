# Copyright 2016 The SAF Authors. All Rights Reserved.
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

cmake_minimum_required(VERSION 3.0)
project(saf)

set(CMAKE_POSITION_INDEPENDENT_CODE YES)

# Include additional CMake files.
include(cmake/build.cmake)
include(cmake/clangformat.cmake)
include(cmake/summary.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

# Determine if the platform is a Tegra.
if (EXISTS /var/libopencv4tegra-repo)
  add_definitions("-D TEGRA")
  SET(TEGRA true)
endif ()

# Determine if the platform is a Mac.
if (APPLE)
  add_definitions("-D APPLE")
  set(CMAKE_MACOSX_RPATH true)
  find_package(vecLib)
  include_directories(SYSTEM ${vecLib_INCLUDE_DIR})
endif ()

# Find general dependencies.
find_package(PkgConfig REQUIRED)
find_package(OpenCV 3.0 REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)
pkg_check_modules(GLOG REQUIRED libglog)
pkg_check_modules(EIGEN3 REQUIRED eigen3)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(Boost REQUIRED COMPONENTS
  date_time graph filesystem iostreams serialization system thread)
find_package(JeMalloc)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
find_package(Protobuf REQUIRED)

# Different backends.
set(BACKENDS "cpu, cuda, opencl")
set(BACKEND "cpu" CACHE STRING "Device backends: ${BACKENDS}")
set(USE_CUDA NO)
set(USE_OPENCL NO)
set(USE_CPU NO)
if (BACKEND STREQUAL "cpu")
  add_definitions("-D CPU_ONLY")
  set(USE_CPU YES)
elseif (BACKEND STREQUAL "cuda")
  add_definitions("-D USE_CUDA")
  set(USE_CUDA YES)
  # See: https://github.com/opencv/opencv/issues/6542
  set(CUDA_USE_STATIC_CUDA_RUNTIME NO)

  find_package(CUDA REQUIRED)
  find_package(Cnmem)

  # Include cblas libraries
  include_directories(SYSTEM ${CUDA_INCLUDE_DIRS})
elseif (BACKEND STREQUAL "opencl")
  add_definitions("-D USE_OPENCL")
  set(USE_OPENCL YES)
  find_package(OpenCL REQUIRED)
else ()
  message(FATAL_ERROR "Backend \"${BACKEND}\" is invalid! Available backends are: ${BACKENDS}")
endif ()

option(USE_CAFFE "Build with Caffe." NO)
if (USE_CAFFE)
  add_definitions("-D USE_CAFFE")

  # Prevent cmake from caching Caffe paths.
  unset(Caffe_FOUND CACHE)
  unset(Caffe_INCLUDE_DIRS CACHE)
  unset(Caffe_LIBRARIES CACHE)

  find_package(Caffe REQUIRED)
  include_directories(SYSTEM ${Caffe_INCLUDE_DIRS})
endif ()

option(USE_TENSORFLOW "Build with TensorFlow." NO)
if (USE_TENSORFLOW)
  add_definitions("-D USE_TENSORFLOW")
  find_package(TensorFlow REQUIRED)
  include_directories(SYSTEM
    ${TensorFlow_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/3rdparty/nsync/public)
endif ()

option(USE_FRCNN "Build with Caffe Faster-RCNN." NO)
if (USE_FRCNN)
  add_definitions("-D USE_FRCNN")
endif ()

option(USE_NCS "Build with NCS." NO)
if (USE_NCS)
  add_definitions("-D USE_NCS")
  find_package(NCS REQUIRED)
  include_directories(${NCS_INCLUDE_DIRS})
endif ()

option(USE_VDMS "Build with VDMS." NO)
if (USE_VDMS)
  add_definitions("-D USE_VDMS")
  find_package(VDMS REQUIRED)
  include_directories(${VDMS_INCLUDE_DIRS})
  include_directories(${VDMS_utils_INCLUDE_DIRS})
endif ()

option(USE_SSD "Build with Caffe SSD." NO)
if (USE_SSD)
  add_definitions("-D USE_SSD")
endif ()

option(USE_PTGRAY "Build with PtGray GigE SDK." NO)
if (USE_PTGRAY)
  add_definitions("-D USE_PTGRAY")
  find_package(PtGray REQUIRED)
  include_directories(SYSTEM ${PtGray_FC_INCLUDE_DIRS})
endif ()

option(USE_VIMBA "Build with AlliedVision Vimba SDK." NO)
if (USE_VIMBA)
  add_definitions("-D USE_VIMBA")
  find_package(Vimba REQUIRED)
  include_directories(SYSTEM ${Vimba_INCLUDE_DIRS})
endif ()

option(USE_RPC "Build with gRPC." NO)
if (USE_RPC)
  add_definitions("-D USE_RPC")
  find_package(GRPC REQUIRED)
  include_directories(
    ${GRPC_INCLUDE_DIRS})
endif ()

option(USE_WEBSOCKET "Build with Websocket." NO)
if (USE_WEBSOCKET)
  add_definitions("-D USE_WEBSOCKET")
  # Include Boost for asio.
  include_directories(${BOOST_INCLUDE_DIR})
endif ()

option(USE_KAFKA "Build with Kafka." NO)
if (USE_KAFKA)
  add_definitions("-D USE_KAFKA")
endif ()

option(USE_MQTT "Build with MQTT." NO)
if (USE_MQTT)
  add_definitions("-D USE_MQTT")
  # Include Boost for asio.
  include_directories(
    3rdparty/mqtt_cpp/include
    ${BOOST_INCLUDE_DIR})
endif ()

option(USE_ISAAC "Build with ISAAC." NO)
if (USE_ISAAC)
  add_definitions("-D USE_ISAAC")
endif ()

option(USE_PYTHON "Build PYTHON module." NO)
if (USE_PYTHON)
  add_definitions("-D USE_PYTHON")
  find_package(PythonLibs 2.7 EXACT REQUIRED)
  include_directories(${PYTHON_INCLUDE_DIRS})
endif ()

option(USE_CVSDK "Build with CVSDK." NO)
if (USE_CVSDK)
  add_definitions("-D USE_CVSDK")
  find_package(CVSDK REQUIRED)
  include_directories(${CVSDK_INCLUDE_DIRS})
endif ()

include_directories(src)
# Common include paths.
include_directories(SYSTEM
  3rdparty
  ${GLOG_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  ${ZMQ_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  ${PROTOBUF_INCLUDE_DIRS})

# Attach the remainder of the build system.
add_subdirectory(src)
add_subdirectory(3rdparty)
add_subdirectory(apps)

# Enable tests.
option(BUILD_TESTS "Build tests." NO)
if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif ()

# Copy example config files in case corresponding manually-created versions do not exist.
file(GLOB EXAMPLE_CONFIG_FILES ${PROJECT_SOURCE_DIR}/config/*.toml.example)
foreach (f ${EXAMPLE_CONFIG_FILES})
  get_filename_component(basename ${f} NAME_WE)
  get_filename_component(dir ${f} DIRECTORY)
  file(COPY ${f} DESTINATION config)
  configure_file(${f} config/${basename}.toml)
endforeach ()

# Copy manually-created config files.
file(GLOB CONFIG_FILES ${PROJECT_SOURCE_DIR}/config/*.toml)
foreach (f ${CONFIG_FILES})
  file(COPY ${f} DESTINATION config)
endforeach ()

saf_print_configuration_summary()
