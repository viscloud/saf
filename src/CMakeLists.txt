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

# Pick source files.
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

file(GLOB_RECURSE CAFFE_SOURCE_FILES
  ${SRC_ROOT}/caffe_*.cpp
  ${SRC_ROOT}/caffe_*.h)

file(GLOB_RECURSE INTEL_CAFFE_SOURCE_FILES
  ${SRC_ROOT}/caffe_yolo_v2_detector.cpp
  ${SRC_ROOT}/caffe_yolo_v2_detector.h
  ${SRC_ROOT}/caffe_feature_extractor.cpp
  ${SRC_ROOT}/caffe_feature_extractor.h)
list(REMOVE_ITEM CAFFE_SOURCE_FILES ${INTEL_CAFFE_SOURCE_FILES})

file(GLOB_RECURSE TENSORFLOW_SOURCE_FILES
  ${SRC_ROOT}/tf_*.cpp
  ${SRC_ROOT}/tf_*.h)

file(GLOB_RECURSE CVSDK_SOURCE_FILES
  ${SRC_ROOT}/cvsdk_*.cpp
  ${SRC_ROOT}/cvsdk_*.h)

file(GLOB_RECURSE RPC_SOURCE_FILES
  ${SRC_ROOT}/operator/rpc/*.cpp
  ${SRC_ROOT}/operator/rpc/*.h)

file(GLOB_RECURSE PTGRAY_SOURCE_FILES
  ${SRC_ROOT}/camera/pgr_*.cpp
  ${SRC_ROOT}/camera/pgr_*.h)

file(GLOB_RECURSE VIMBA_SOURCE_FILES
  ${SRC_ROOT}/camera/vimba_*.cpp
  ${SRC_ROOT}/camera/vimba_*.h)

file(GLOB_RECURSE FRCNN_SOURCE_FILES
  ${SRC_ROOT}/frcnn_*.cpp
  ${SRC_ROOT}/frcnn_*.h)

file(GLOB_RECURSE NCS_SOURCE_FILES
  ${SRC_ROOT}/ncs.h
  ${SRC_ROOT}/ncs_*.cpp
  ${SRC_ROOT}/ncs_*.h)

file(GLOB_RECURSE INTEL_CAFFE_SSD_SOURCE_FILES
  ${SRC_ROOT}/caffe*ssd_*.cpp
  ${SRC_ROOT}/caffe*ssd_*.h
  ${SRC_ROOT}/ssd_*.cpp
  ${SRC_ROOT}/ssd_*.h)
list(REMOVE_ITEM CAFFE_SOURCE_FILES ${INTEL_CAFFE_SSD_SOURCE_FILES})

file(GLOB_RECURSE CVSDK_SSD_SOURCE_FILES
  ${SRC_ROOT}/cvsdk*ssd_*.cpp
  ${SRC_ROOT}/cvsdk*ssd_*.h)
list(REMOVE_ITEM CVSDK_SOURCE_FILES ${CVSDK_SSD_SOURCE_FILES})

file(GLOB_RECURSE DLIB_SOURCE_FILES
  ${SRC_ROOT}/dlib_*.cpp
  ${SRC_ROOT}/dlib_*.h)

file(GLOB_RECURSE PYTHON_API_SOURCE_FILES
  ${SRC_ROOT}/api/python/*.cpp
  ${SRC_ROOT}/api/python/*.h)

# Select all source files, then remove files pertaining to optional features.
file(GLOB_RECURSE SAF_SOURCE_FILES
  ${SRC_ROOT}/*.cpp
  ${SRC_ROOT}/*.h)
list(REMOVE_ITEM SAF_SOURCE_FILES
  ${RPC_SOURCE_FILES}
  ${CAFFE_SOURCE_FILES}
  ${INTEL_CAFFE_SOURCE_FILES}
  ${TENSORFLOW_SOURCE_FILES}
  ${CVSDK_SOURCE_FILES}
  ${PTGRAY_SOURCE_FILES}
  ${VIMBA_SOURCE_FILES}
  ${FRCNN_SOURCE_FILES}
  ${NCS_SOURCE_FILES}
  ${INTEL_CAFFE_SSD_SOURCE_FILES}
  ${CVSDK_SSD_SOURCE_FILES}
  ${DLIB_SOURCE_FILES}
  ${PYTHON_API_SOURCE_FILES})

# Pick libraries. Boost's graph library is header-only, so it does not need to
# be linked.
set(SAF_LIBRARIES
  ${OpenCV_LIBRARIES}
  ${GLOG_LIBRARIES}
  ${GSTREAMER_LIBRARIES}
  ${ZMQ_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_IOSTREAMS_LIBRARY}
  ${Boost_SERIALIZATION_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT})

if (USE_CAFFE)
  list(APPEND SAF_SOURCE_FILES ${CAFFE_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${Caffe_LIBRARIES})

  if (USE_CUDA AND Cnmem_FOUND)
    list(APPEND SAF_LIBRARIES ${Cnmem_LIBRARIES})
  endif ()

  # Only include source files that require Intel Caffe if we are using it.
  if (HAVE_INTEL_CAFFE)
    list(APPEND SAF_SOURCE_FILES ${INTEL_CAFFE_SOURCE_FILES})
  endif ()

  # Assemble Caffe compile flags. This will be used below, after we have created
  # the "saf" target.
  set(CAFFE_CFLAGS "-MMD -MP -pthread -fPIC -DUSE_OPENCV")

  if (USE_CUDA)
    set(CAFFE_CFLAGS "${CAFFE_CFLAGS} -DUSE_CUDNN")
    if (Cnmem_FOUND)
      set(CAFFE_CFLAGS "${CAFFE_CFLAGS} -DUSE_CNMEM")
    endif ()
  endif ()
endif ()

if (USE_TENSORFLOW)
  list(APPEND SAF_SOURCE_FILES ${TENSORFLOW_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${TensorFlow_LIBRARIES})
endif ()

if (USE_PTGRAY)
  list(APPEND SAF_SOURCE_FILES ${PTGRAY_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${PtGray_LIBRARIES})
endif ()

if (USE_VIMBA)
  list(APPEND SAF_SOURCE_FILES ${VIMBA_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${Vimba_LIBRARIES})
endif ()

if (USE_RPC)
  file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/operator/rpc/*.proto")
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
  protobuf_generate_grpc_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILES})
  # Silence compiler warnings in automatically-generated source files.
  foreach (f ${PROTO_SRCS} ${GRPC_SRCS})
    get_filename_component(ff ${f} NAME)
    set_source_files_properties(${ff} PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
  endforeach ()
  list(APPEND SAF_SOURCE_FILES ${RPC_SOURCE_FILES})
  list(APPEND SAF_SOURCE_FILES ${PROTO_SRCS} ${PROTO_HDRS})
  list(APPEND SAF_SOURCE_FILES ${GRPC_SRCS} ${GRPC_HDRS})
  list(APPEND SAF_LIBRARIES ${PROTOBUF_LIBRARIES})
  list(APPEND SAF_LIBRARIES ${GRPC_LIBRARIES})
endif ()

if (USE_FRCNN)
  list(APPEND SAF_SOURCE_FILES ${FRCNN_SOURCE_FILES})
endif ()

list(APPEND SAF_SOURCE_FILES ${DLIB_SOURCE_FILES})
list(APPEND SAF_LIBRARIES dlib)

if (USE_KAFKA)
  list(APPEND SAF_LIBRARIES rdkafka)
  list(APPEND SAF_LIBRARIES rdkafka++)
endif ()

if (USE_NCS)
  list(APPEND SAF_SOURCE_FILES ${NCS_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${NCS_LIBRARIES})
endif ()

if (USE_VDMS)
  message(STATUS "VDMS enabled but not used in this version of SAF!")
  list(APPEND SAF_LIBRARIES ${VDMS_LIBRARIES} ${VDMS_utils_LIBRARIES})
endif ()

# Building with SSD requires Intel Caffe.
if (USE_SSD)
  if (HAVE_INTEL_CAFFE)
    list(APPEND SAF_SOURCE_FILES ${INTEL_CAFFE_SSD_SOURCE_FILES})
  else ()
    message(STATUS "Disabling Caffe SSD because not using Intel Caffe!")
  endif ()
endif ()

# Protobuf is a top-level dependency.
list(APPEND SAF_LIBRARIES ${PROTOBUF_LIBRARIES})

if (USE_WEBSOCKET)
  list(APPEND SAF_LIBRARIES cpprest)
endif ()

# Build Python API.
if (USE_PYTHON)
  list(APPEND SAF_LIBRARIES
    ${PYTHON_LIBRARY})
  add_library(safpy SHARED ${PYTHON_API_SOURCE_FILES})

  target_link_libraries(safpy
      ${PYTHON_LIBRARIES}
      ${SAF_LIBRARIES}
      saf
      RTTR::Core
      pbcvt)

  add_build_reqs(safpy)
  set_target_properties(safpy PROPERTIES PREFIX "" OUTPUT_NAME safpy)

  # Use python2 to be safe because the pyboostcvconverter is configured for
  # Python 2.
  execute_process(COMMAND python2 -c "from distutils.sysconfig import get_python_lib; print get_python_lib()"
          OUTPUT_VARIABLE PYTHON_SITE
          OUTPUT_STRIP_TRAILING_WHITESPACE)
  add_custom_target(install_python
      COMMAND ln -sf
      ${CMAKE_BINARY_DIR}/src/safpy.so ${PYTHON_SITE}/safpy.so COMMENT "Safpy linked at ${PYTHON_SITE}." DEPENDS safpy)
  add_custom_target(uninstall_python
      COMMAND rm -f ${PYTHON_SITE}/safpy.so
      COMMENT "Safpy unlinked at${PYTHON_SITE}." DEPENDS safpy)
endif ()

if (USE_MQTT)
  # MQTT requires SSL.
  list(APPEND SAF_LIBRARIES ssl crypto)
endif ()

if (USE_CVSDK)
  list(APPEND SAF_SOURCE_FILES ${CVSDK_SOURCE_FILES})
  list(APPEND SAF_LIBRARIES ${CVSDK_LIBRARIES} ${CVSDK_EXT_LIBRARIES})

  if (USE_SSD)
    list(APPEND SAF_SOURCE_FILES ${CVSDK_SSD_SOURCE_FILES})
  endif ()
endif ()

# Generate protobuf sources.
file(GLOB PROTO_FILES "proto/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
protobuf_generate_python(PROTO_PYTHON ${PROTO_FILES})
include_directories(SYSTEM ${CMAKE_CURRENT_BINARY_DIR})
foreach (f ${PROTO_SRCS} ${PROTO_HDRS})
  get_filename_component(ff ${f} NAME)
  set_source_files_properties(${ff} PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endforeach ()
list(APPEND SAF_SOURCE_FILES ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_PYTHON})

# Enable these to help debug build problems.
# message(STATUS "SAF libraries: ${SAF_LIBRARIES}")
# message(STATUS "SAF sources: ${SAF_SOURCE_FILES}")

add_library(saf SHARED ${SAF_SOURCE_FILES})
target_link_libraries(saf ${SAF_LIBRARIES})
add_build_reqs(saf)

# Set Caffe compile flags.
if (USE_CAFFE)
  get_target_property(TEMP saf COMPILE_FLAGS)
  if (TEMP STREQUAL "TEMP-NOTFOUND")
    # Set to empty string.
    set(TEMP "")
  endif ()
  # Append our values.
  set(TEMP "${CAFFE_CFLAGS} ${TEMP}")
  set_target_properties(saf PROPERTIES COMPILE_FLAGS ${TEMP})
endif ()
