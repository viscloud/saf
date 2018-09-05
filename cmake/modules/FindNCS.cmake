
unset(NCS_FOUND)

find_path(NCS_INCLUDE_DIRS NAMES ncs.h mvnc.h
    HINTS
    /usr/local/include)

find_library(NCS_LIBRARIES NAMES ncs mvnc
    HINTS
    /usr/local/lib)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NCS DEFAULT_MSG
    NCS_INCLUDE_DIRS NCS_LIBRARIES)

if (NCS_LIBRARIES AND NCS_INCLUDE_DIRS)
    set(NCS_FOUND YES)
endif ()
