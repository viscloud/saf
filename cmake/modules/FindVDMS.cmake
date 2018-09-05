
# Visual Data Management System (VDMS)
unset(VDMS_FOUND)

find_path(VDMS_INCLUDE_DIRS NAMES client/cpp/VDMSClient.h
  HINTS
  ${VDMS_HOME})

find_path(VDMS_utils_INCLUDE_DIRS NAMES comm/Connection.h
  HINTS
  ${VDMS_HOME}/utils/include)

find_library(VDMS_LIBRARIES NAMES vdms-client
  HINTS
  ${VDMS_HOME}/client/cpp)

find_library(VDMS_utils_LIBRARIES NAMES vdms-utils
  HINTS
  ${VDMS_HOME}/utils)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(VDMS DEFAULT_MSG
    VDMS_INCLUDE_DIRS VDMS_utils_INCLUDE_DIRS VDMS_LIBRARIES VDMS_utils_LIBRARIES)

if (VDMS_LIBRARIES AND VDMS_INCLUDE_DIRS)
  set(VDMS_FOUND YES)
endif ()
