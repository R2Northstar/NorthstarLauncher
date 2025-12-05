# Standard FIND_PACKAGE module for libuv, sets the following variables:
#   - LIBUV_FOUND
#   - LIBUV_INCLUDE_DIRS (only if LIBUV_FOUND)
#   - LIBUV_LIBRARIES (only if LIBUV_FOUND)

# Try to find the header
FIND_PATH(LIBUV_INCLUDE_DIR NAMES uv.h)

# Try to find the library
FIND_LIBRARY(LIBUV_LIBRARIES NAMES uv libuv)

# Handle the QUIETLY/REQUIRED arguments, set LIBUV_FOUND if all variables are
# found
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUV
                                  REQUIRED_VARS
                                  LIBUV_LIBRARIES
                                  LIBUV_INCLUDE_DIR)

# Hide internal variables
MARK_AS_ADVANCED(LIBUV_INCLUDE_DIR LIBUV_LIBRARIES)

# Set standard variables
IF(LIBUV_FOUND)
    SET(LIBUV_INCLUDE_DIRS "${LIBUV_INCLUDE_DIR}")
    SET(LIBUV_LIBRARIES "${LIBUV_LIBRARIES}")
ENDIF()
