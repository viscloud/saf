# Find Vimba SDK. These variables will be set:
#
# Vimba_FOUND        - If Vimba FlyCapture SDK is found
# Vimba_INCLUDE_DIRS - Header directories for Vimba FlyCapTure SDK
# Vimba_LIBRARIES    - Libraries for Vimba FlyCapture SDK

unset(Vimba_FOUND)

EXECUTE_PROCESS(
  COMMAND uname -m
  COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)

if (${ARCHITECTURE} STREQUAL "aarch64")
  set(ARCHITECTURE "arm_64")
endif()

message(STATUS "Architecture: ${ARCHITECTURE}")

find_path(Vimba_INCLUDE_DIRS NAMES
  VimbaCPP/Include/
  VimbaC/Include/
  VimbaImageTransform/Include/
  HINTS
  ${VIMBA_HOME}
  ${VIMBA_HOME}/include)

find_library(Vimba_CPP_LIBRARY NAMES VimbaCPP
  HINTS
  ${VIMBA_HOME}
  ${VIMBA_HOME}/VimbaCPP/DynamicLib/${ARCHITECTURE}bit)

find_library(Vimba_C_LIBRARY NAMES VimbaC
  HINTS
  ${VIMBA_HOME}
  ${VIMBA_HOME}/VimbaC/DynamicLib/${ARCHITECTURE}bit)

find_library(Vimba_ImageTransform_LIBRARY NAMES VimbaImageTransform
  HINTS
  ${VIMBA_HOME}
  ${VIMBA_HOME}/VimbaImageTransform/DynamicLib/${ARCHITECTURE}bit)

set(Vimba_LIBRARIES
  ${Vimba_CPP_LIBRARY}
  ${Vimba_C_LIBRARY}
  ${Vimba_ImageTransform_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Vimba DEFAULT_MSG
  Vimba_INCLUDE_DIRS Vimba_LIBRARIES)

if (Vimba_INCLUDE_DIRS AND Vimba_LIBRARIES)
  set(Vimba_FOUND YES)
endif ()
