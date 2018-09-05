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

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

if (USE_RPC)
  include_directories(SYSTEM ${CMAKE_BINARY_DIR}/src)
endif ()

# Macro to get sub-directories.
macro(subdirlist REAULT CURDIR)
  file(GLOB CHILDREN RELATIVE ${CURDIR} ${CURDIR}/*)
  set(DIRLIST "")
  foreach (CHILD ${CHILDREN})
    if (IS_DIRECTORY ${CURDIR}/${CHILD})
      list(APPEND DIRLIST ${CHILD})
    endif ()
  endforeach ()
  set(${REAULT} ${DIRLIST})
endmacro()

set(APPS_ROOT ${PROJECT_SOURCE_DIR}/apps/)
subdirlist(APP_DIRS ${APPS_ROOT})

add_custom_target(apps)

# Macro for build a single app. Used by apps that have their own CMakeLists.txt
# files.
macro(build_app APP)
  if (JEMALLOC_FOUND)
    target_link_libraries(${APP} ${JAMALLOC_LIBRARIES})
  endif ()
  target_link_libraries(${APP} saf ${Boost_PROGRAM_OPTIONS_LIBRARY})
  add_dependencies(apps ${APP})
  add_build_reqs(${APP})
endmacro()

# For each app, either invoke its custom build system or build it ourselves.
foreach (APP_DIR ${APP_DIRS})
  if (EXISTS ${PROJECT_SOURCE_DIR}/apps/${APP_DIR}/CMakeLists.txt)
    add_subdirectory(${APP_DIR})
  else ()
    file(GLOB APP_SRCS ${APP_DIR}/*.cpp ${APP_DIR}/*.h ${APP_DIR}/*.c)
    get_filename_component(APP_NAME ${APP_DIR} NAME_WE)
    add_executable(${APP_NAME} EXCLUDE_FROM_ALL ${APP_SRCS})
    if (JEMALLOC_FOUND)
      target_link_libraries(${APP_NAME} ${JAMALLOC_LIBRARIES})
    endif ()
    target_link_libraries(${APP_NAME} saf ${Boost_PROGRAM_OPTIONS_LIBRARY})
    add_build_reqs(${APP_NAME})

    get_filename_component(APP_NAME ${APP_DIR} NAME_WE)
    add_dependencies(apps ${APP_NAME})
  endif ()
endforeach ()
