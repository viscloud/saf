
unset(CVSDK_FOUND)

find_path(CVSDK_INCLUDE_DIRS NAMES ie_version.hpp inference_engine.hpp
  HINTS /opt/intel/computer_vision_sdk ${CVSDK_HOME}
  PATH_SUFFIXES deployment_tools/inference_engine/include)

find_library(CVSDK_LIBRARIES NAMES inference_engine
  HINTS /opt/intel/computer_vision_sdk ${CVSDK_HOME}
  PATH_SUFFIXES deployment_tools/inference_engine/lib/ubuntu_16.04/intel64)

find_library(CVSDK_EXT_LIBRARIES NAMES cpu_extension
  HINTS /opt/intel/computer_vision_sdk ${CVSDK_HOME}
  PATH_SUFFIXES deployment_tools/inference_engine/samples/build/intel64/Release/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CVSDK DEFAULT_MSG CVSDK_INCLUDE_DIRS CVSDK_LIBRARIES)

if (CVSDK_LIBRARIES AND CVSDK_EXT_LIBRARIES AND CVSDK_INCLUDE_DIRS)
  set(CVSDK_FOUND YES)
endif ()
