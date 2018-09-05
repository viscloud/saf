
unset(Caffe_FOUND)

find_path(Caffe_INCLUDE_DIRS NAMES caffe/caffe.hpp
  HINTS
  ${CAFFE_HOME}/include
  /usr/local/include)

find_library(Caffe_LIBRARIES NAMES caffe-nv caffe
  HINTS
  ${CAFFE_HOME}/lib
  /usr/local/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Caffe DEFAULT_MSG
  Caffe_INCLUDE_DIRS Caffe_LIBRARIES)

if (Caffe_LIBRARIES AND Caffe_INCLUDE_DIRS)
  set(Caffe_FOUND YES)
  INCLUDE(CheckTypeSize)
  set(CMAKE_EXTRA_INCLUDE_FILES "${Caffe_INCLUDE_DIRS}/caffe/proto/caffe.pb.h")
  CHECK_TYPE_SIZE("caffe::ResizeParameter" CAFFE_RESIZE_PARAMETER LANGUAGE CXX)
  # If caffe::ResizeParameter exists in the Caffe protobuf file, assume that we
  # have Intel Caffe (caffe-mkl)
  set(HAVE_INTEL_CAFFE ${HAVE_CAFFE_RESIZE_PARAMETER} CACHE INTERNAL "Using Intel Caffe?")
endif ()
