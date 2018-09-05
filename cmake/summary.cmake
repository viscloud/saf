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

# SAF status report function. (Borrowed from Caffe)
# Automatically align right column and selects text based on condition.
# Usage:
#   saf_status(<text>)
#   saf_status(<heading> <value1> [<value2> ...])
#   saf_status(<heading> <condition> THEN <text for TRUE> ELSE <text for FALSE> )
function(saf_status text)
  set(status_cond)
  set(status_then)
  set(status_else)

  set(status_current_name "cond")
  foreach (arg ${ARGN})
    if (arg STREQUAL "THEN")
      set(status_current_name "then")
    elseif (arg STREQUAL "ELSE")
      set(status_current_name "else")
    else ()
      list(APPEND status_${status_current_name} ${arg})
    endif ()
  endforeach ()

  if (DEFINED status_cond)
    set(status_placeholder_length 23)
    string(RANDOM LENGTH ${status_placeholder_length} ALPHABET " " status_placeholder)
    string(LENGTH "${text}" status_text_length)
    if (status_text_length LESS status_placeholder_length)
      string(SUBSTRING "${text}${status_placeholder}" 0 ${status_placeholder_length} status_text)
    elseif (DEFINED status_then OR DEFINED status_else)
      message(STATUS "${text}")
      set(status_text "${status_placeholder}")
    else ()
      set(status_text "${text}")
    endif ()

    if (DEFINED status_then OR DEFINED status_else)
      if (${status_cond})
        string(REPLACE ";" " " status_then "${status_then}")
        string(REGEX REPLACE "^[ \t]+" "" status_then "${status_then}")
        message(STATUS "${status_text} ${status_then}")
      else ()
        string(REPLACE ";" " " status_else "${status_else}")
        string(REGEX REPLACE "^[ \t]+" "" status_else "${status_else}")
        message(STATUS "${status_text} ${status_else}")
      endif ()
    else ()
      string(REPLACE ";" " " status_cond "${status_cond}")
      string(REGEX REPLACE "^[ \t]+" "" status_cond "${status_cond}")
      message(STATUS "${status_text} ${status_cond}")
    endif ()
  else ()
    message(STATUS "${text}")
  endif ()
endfunction()


################################################################################################
# Function for fetching SAF version from git and headers
# Usage:
#   saf_extract_saf_version()
function(saf_extract_saf_version)
  set(SAF_GIT_VERSION "unknown")
  find_package(Git)
  if (GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
      ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      OUTPUT_VARIABLE SAF_GIT_VERSION
      RESULT_VARIABLE __git_result)
    if (NOT ${__git_result} EQUAL 0)
      set(SAF_GIT_VERSION "unknown")
    endif ()
  endif ()

  set(SAF_GIT_VERSION ${SAF_GIT_VERSION} PARENT_SCOPE)
  set(SAF_VERSION "<TODO> (SAF doesn't declare its version in headers)" PARENT_SCOPE)

  # saf_parse_header(${saf_INCLUDE_DIR}/saf/version.hpp saf_VERSION_LINES saf_MAJOR saf_MINOR saf_PATCH)
  # set(saf_VERSION "${saf_MAJOR}.${saf_MINOR}.${saf_PATCH}" PARENT_SCOPE)

  # or for #define saf_VERSION "x.x.x"
  # saf_parse_header_single_define(saf ${saf_INCLUDE_DIR}/saf/version.hpp saf_VERSION)
  # set(saf_VERSION ${saf_VERSION_STRING} PARENT_SCOPE)

endfunction()

################################################################################################
# Function merging lists of compiler flags to single string.
# Usage:
#   saf_merge_flag_lists(out_variable <list1> [<list2>] [<list3>] ...)
function(saf_merge_flag_lists out_var)
  set(__result "")
  foreach (__list ${ARGN})
    foreach (__flag ${${__list}})
      string(STRIP ${__flag} __flag)
      set(__result "${__result} ${__flag}")
    endforeach ()
  endforeach ()
  string(STRIP ${__result} __result)
  set(${out_var} ${__result} PARENT_SCOPE)
endfunction()

################################################################################################
# Prints accumulated saf configuration summary
# Usage:
#   saf_print_configuration_summary()

function(saf_print_configuration_summary)
  saf_extract_saf_version()
  list(GET OpenCV_INCLUDE_DIRS 0 OPENCV_VERSION)
  list(GET Protobuf_LIBRARIES 0 Protobuf_LIB)

  saf_merge_flag_lists(__flags_rel CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS)
  saf_merge_flag_lists(__flags_deb CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS)

  saf_status("")
  saf_status("********************** SAF Configuration Summary **********************")
  saf_status("General:")
  saf_status("  Git               :   ${SAF_GIT_VERSION}")
  saf_status("  System            :   ${CMAKE_SYSTEM_NAME}")
  saf_status("  On Tegra          : " TEGRA THEN "Yes" ELSE "No")
  saf_status("  Compiler          :   ${CMAKE_CXX_COMPILER} (ver. ${CMAKE_CXX_COMPILER_VERSION})")
  saf_status("  Release CXX flags :   ${__flags_rel}")
  saf_status("  Debug CXX flags   :   ${__flags_deb}")
  saf_status("  Build type        :   ${CMAKE_BUILD_TYPE}")
  saf_status("  Build tests       : " ${BUILD_TESTS} THEN "Yes" ELSE "No")
  saf_status("")
  saf_status("Options:")
  saf_status("  BACKEND           :   ${BACKEND}")
  saf_status("  USE_CVSDK         : " ${USE_CVSDK} THEN "Yes" ELSE "No")
  saf_status("  USE_CAFFE         : " ${USE_CAFFE} THEN "Yes" ELSE "No")
  saf_status("  USE_FRCNN         : " ${USE_FRCNN} THEN "Yes" ELSE "No")
  saf_status("  USE_ISAAC         : " ${USE_ISAAC} THEN "Yes" ELSE "No")
  saf_status("  USE_KAFKA         : " ${USE_KAFKA} THEN "Yes" ELSE "No")
  saf_status("  USE_MQTT          : " ${USE_MQTT} THEN "Yes" ELSE "No")
  saf_status("  USE_NCS           : " ${USE_NCS} THEN "Yes" ELSE "No")
  saf_status("  USE_PTGRAY        : " ${USE_PTGRAY} THEN "Yes" ELSE "No")
  saf_status("  USE_PYTHON        : " ${USE_PYTHON} THEN "Yes" ELSE "No")
  saf_status("  USE_RPC           : " ${USE_RPC} THEN "Yes" ELSE "No")
  saf_status("  USE_SSD           : " ${USE_SSD} THEN "Yes" ELSE "No")
  saf_status("  USE_TENSORFLOW    : " ${USE_TENSORFLOW} THEN "Yes" ELSE "No")
  saf_status("  USE_WEBSOCKET     : " ${USE_WEBSOCKET} THEN "Yes" ELSE "No")
  saf_status("  USE_VDMS          : " ${USE_VDMS} THEN "Yes" ELSE "No")
  saf_status("  USE_VIMBA         : " ${USE_VIMBA} THEN "Yes" ELSE "No")
  saf_status("")
  saf_status("Frameworks:")
  saf_status("  Caffe             : " Caffe_FOUND THEN "Yes (${Caffe_INCLUDE_DIRS})" "[Found Intel Caffe = ${HAVE_INTEL_CAFFE}]" ELSE "No")
  saf_status("  TensorFlow        : " TensorFlow_FOUND THEN "Yes (${TENSORFLOW_HOME})" ELSE "No")
  saf_status("")
  saf_status("Dependencies:")
  saf_status("  Boost             :   Yes (ver. ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}, ${Boost_SYSTEM_LIBRARY})")
  saf_status("  OpenCV            :   Yes (ver. ${OpenCV_VERSION}, ${OPENCV_VERSION})")
  saf_status("  Eigen             :   Yes (ver. ${EIGEN3_VERSION}, ${EIGEN3_INCLUDE_DIRS})")
  saf_status("  GStreamer         :   Yes (ver. ${GSTREAMER_gstreamer-1.0_VERSION}, ${GSTREAMER_LIBRARIES})")
  saf_status("  glog              :   Yes (ver. ${GLOG_VERSION}, ${GLOG_LIBRARIES})")
  saf_status("  ZeroMQ            :   Yes (ver. ${ZMQ_VERSION}, ${ZMQ_LIBRARIES})")
  saf_status("  VecLib            : " VECLIB_FOUND THEN "Yes (${vecLib_INCLUDE_DIR})" ELSE "No")
  saf_status("  CUDA              : " CUDA_FOUND THEN "Yes (ver. ${CUDA_VERSION_STRING})" ELSE "No")
  saf_status("  Cnmem             : " Cnmem_FOUND THEN "Yes (${Cnmem_LIBRARIES})" ELSE "No")
  saf_status("  OpenCL            : " OPENCL_FOUND THEN "Yes (ver. ${OPENCL_VERSION_STRING})" ELSE "No")
  saf_status("  Protobuf          : " PROTOBUF_FOUND THEN " Yes (ver. ${Protobuf_VERSION}, ${Protobuf_LIB})" ELSE "No")
  if (USE_RPC)
    saf_status("  gRPC              : " GRPC_LIBRARIES THEN " Yes (${GRPC_LIBRARIES})" ELSE "No")
  endif ()
  saf_status("  JeMalloc          : " JEMALLOC_FOUND THEN " Yes (${JEMALLOC_LIBRARIES})" ELSE "No")
  saf_status("  FlyCapture        : " PtGray_FOUND THEN " Yes (${PtGray_LIBRARIES})" ELSE "No")
  saf_status("  Vimba             : " Vimba_FOUND THEN " Yes (${Vimba_LIBRARIES})" ELSE "No")
  saf_status("  Python API        : " ${USE_PYTHON} THEN "Yes" ELSE "No")
  saf_status("")
  saf_status("Install:")
  saf_status(" Install path : ${CMAKE_INSTALL_PREFIX}")
  saf_status("")
endfunction()
