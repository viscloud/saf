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

# Prepare Git submodules.
execute_process(COMMAND git submodule update --init
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_TESTS)
  add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif ()

set(USE_SSE4_INSTRUCTIONS ON CACHE BOOL "Enable SSE4 instructions" FORCE)
add_subdirectory(dlib EXCLUDE_FROM_ALL)

if (USE_KAFKA)
  set(RDKAFKA_BUILD_STATIC ON CACHE BOOL "Enable RdKafka static linking" FORCE)
  add_subdirectory(librdkafka EXCLUDE_FROM_ALL)
endif ()

add_subdirectory(litesql EXCLUDE_FROM_ALL)

if (USE_PYTHON)
  # git-reset the specified dir to the specifed state (i.e., tag, commit, etc.).
  macro(reset DIR VERSION)
    execute_process(
      COMMAND git reset --hard ${VERSION}
      WORKING_DIRECTORY ${DIR}
      RESULT_VARIABLE ERR_CODE)
    if (NOT "${ERR_CODE}" STREQUAL "0")
      message(FATAL_ERROR "Error when resetting ${DIR} to ${VERSION}.")
    endif ()
  endmacro()

  macro(reset_rttr)
    reset(${CMAKE_CURRENT_SOURCE_DIR}/rttr "v0.9.6")
  endmacro()

  # git-apply the provided patch (which should be located in
  # <SAF root>/patches) to the specified dir.
  macro(patch DIR patch_NAME)
    execute_process(
      COMMAND git apply ${CMAKE_SOURCE_DIR}/patches/${patch_NAME}
      WORKING_DIRECTORY ${DIR}
      RESULT_VARIABLE ERR_CODE)
    if (NOT "${ERR_CODE}" STREQUAL "0")
      message(FATAL_ERROR "Error when patching ${DIR}.")
    endif ()
  endmacro()

  # Add the RTTR submodule. We need to patch the RTTR build system in order to
  # make it support being built as part of another project. First, we reset RTTR
  # to our selected version. Second, we apply the patch. Third, we add the RTTR
  # directory to our project. Fourth, we reset RTTR again to undo the patch.
  message(STATUS "Patching RTTR...")
  reset_rttr()
  patch(${CMAKE_CURRENT_SOURCE_DIR}/rttr 0001-Set-LICENSE_FILE-in-relation-to-current-CMakeLists.t.patch)
  message(STATUS "Patched RTTR")

  # Disable unnecessary RTTR components.
  set(BUILD_UNIT_TESTS OFF CACHE BOOL "Disable RTTR tests.")
  set(BUILD_EXAMPLES OFF CACHE BOOL "Disable RTTR examples.")
  set(BUILD_DOCUMENTATION OFF CACHE BOOL "Disable RTTR documentation.")

  # We record the value of CMAKE_INSTALL_PREFIX because RTTR overrides it, which
  # is a terrible practice.
  set(CMAKE_INSTALL_PREFIX_BACKUP ${CMAKE_INSTALL_PREFIX})
  add_subdirectory(rttr EXCLUDE_FROM_ALL)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX_BACKUP} PARENT_SCOPE)
  set(RTTR_FOUND YES PARENT_SCOPE)

  message(STATUS "Un-patching RTTR...")
  reset_rttr()
  message(STATUS "Un-patched RTTR")

  # Add the pyboostcvconverter (PBCC) submodule. Set the desired version to
  # Python 2.
  set(PYTHON_DESIRED_VERSION "2.X" CACHE STRING "Use Python 2")
  add_subdirectory(pyboostcvconverter EXCLUDE_FROM_ALL)
endif ()
