
# Simple library to help the Deep Learning frameworks manage CUDA memory.
# https://github.com/NVIDIA/cnmem
unset(Cnmem_FOUND)

find_library(Cnmem_LIBRARIES NAMES cnmem
        HINTS
        /usr/local/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cnmem DEFAULT_MSG
        Cnmem_LIBRARIES)

if (Cnmem_LIBRARIES)
  set(Cnmem_FOUND YES)
endif ()
