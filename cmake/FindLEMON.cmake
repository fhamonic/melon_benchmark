# ##############################################################################
# CMake module for the Lemon graph library
# ##############################################################################
# Given a path to a directory where Lemon is installed, this module defines the
# paths for the include directories as well as the libraries.
#
# The user must define LEMON_DIR for this module to work.
#
# ##############################################################################

# Determine path to the Lemon graph library
find_path(
  # Variable name
  LEMON_DIR
  # Paths that should be available relative to the path searched for
	# (helps CMake to validate that it found the right path)
  NAMES "include/lemon/lp_base.h"
  # Documentation string for this variable
  DOC "Path to Lemon graph library"
)

# Determine the include paths
find_path(
  # Variable name
  LEMON_INCLUDE_DIRS
  # Paths that should be available relative to the path searched for
	# (helps CMake to validate that it found the right path)
  NAMES "lemon/full_graph.h"
  # Paths that CMake will consider when searching
  PATHS "${LEMON_DIR}/include"
  # Documentation string for this variable
  DOC "Lemon graph library include path"
)

# Determine path to the library
find_library(
  # Variable name
  LEMON_LIBRARIES
  # Possible names of the library
  NAMES
    emon          # Note: The missing 'l' for "lemon" is not a mistake. The
    libemon       #       library is indeed named like that!
    libemon.a
  # Paths that CMake will consider when searching
  PATHS
    "${LEMON_DIR}/lib"
  # Documentation string for this variable
  DOC "Path to compiled Lemon graph library"
)

message(STATUS "Path to Lemon directory: ${LEMON_DIR}")
message(STATUS "Path to Lemon libraries: ${LEMON_LIBRARIES}")
message(STATUS "Path to Lemon include directory: ${LEMON_INCLUDE_DIRS}")

if(
  NOT LEMON_DIR
  OR NOT LEMON_LIBRARIES
  OR NOT LEMON_INCLUDE_DIRS
)
  message(
    FATAL_ERROR
    "Could not locate Lemon graph library. Have you specified its path? On "
    "command line use -DLEMON_DIR=<PATH>"
  )
endif()
