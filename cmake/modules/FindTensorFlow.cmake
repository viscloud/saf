
# Locate the TensorFlow library and include directories
unset(TensorFlow_FOUND)

set (TensorFlow_INCLUDE_DIR_BAZEL ${TENSORFLOW_HOME}/bazel-genfiles)
set (TensorFlow_INCLUDE_DIR_PROTOBUF ${TENSORFLOW_HOME}/bazel-tensorflow/external/protobuf/src)
set (TensorFlow_INCLUDE_DIR_TENSORFLOW ${TENSORFLOW_HOME})

link_directories(${TENSORFLOW_HOME}/bazel-out/host/bin/tensorflow/core/_objs/protos_all_cc/tensorflow/core/protobuf)

find_library(TensorFlow_LIBRARY NAMES tensorflow_cc
  HINTS
  ${TENSORFLOW_HOME}/bazel-bin/tensorflow
  /usr/lib
  /usr/local/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TensorFlow DEFAULT_MSG
	TensorFlow_INCLUDE_DIR_BAZEL TensorFlow_INCLUDE_DIR_TENSORFLOW TensorFlow_LIBRARY)

if (TensorFlow_LIBRARY AND TensorFlow_INCLUDE_DIR_BAZEL AND TensorFlow_INCLUDE_DIR_TENSORFLOW)
  set(TensorFlow_FOUND YES)
  set(TensorFlow_LIBRARIES ${TensorFlow_LIBRARY})
  set(TensorFlow_INCLUDE_DIRS ${TensorFlow_INCLUDE_DIR_BAZEL} ${TensorFlow_INCLUDE_DIR_TENSORFLOW} ${TensorFlow_INCLUDE_DIR_PROTOBUF})
endif ()

# hide locals from GUI
mark_as_advanced(TensorFlow_INCLUDE_DIR TensorFlow_LIBRARY)
