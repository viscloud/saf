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

file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)
set(PROJECT_THIRD_PARTY_DIR "3rdparty")
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
  string(FIND ${SOURCE_FILE} ${PROJECT_THIRD_PARTY_DIR} PROJECT_THIRD_PARTY_DIR_FOUND)
  if (NOT ${PROJECT_THIRD_PARTY_DIR_FOUND} EQUAL -1)
    list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
  endif ()
endforeach ()

add_custom_target(clangformat
  COMMAND
  clang-format
  -style=file
  -i ${ALL_SOURCE_FILES})
