# Find PtGray SDK. These variables will be set:
#
# PtGray_FOUND        - If PtGray FlyCapture SDK is found
# PtGray_INCLUDE_DIRS - Header directories for PtGray FlyCapTure SDK
# PtGray_LIBRARIES    - Libraries for PtGray FlyCapture SDK

unset(PtGray_FOUND)

find_path(PtGray_INCLUDE_DIRS NAMES flycapture/FlyCapture2.h flycapture/Camera.h
  HINTS
  ${PtGray_HOME}/include)

find_library(PtGray_LIBRARIES NAMES flycapture
  HINTS
  ${PtGray_HOME}/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PtGray DEFAULT_MSG
  PtGray_INCLUDE_DIRS PtGray_LIBRARIES)

if (PtGray_INCLUDE_DIRS AND PtGray_LIBRARIES)
  set(PtGray_FOUND YES)
endif ()
