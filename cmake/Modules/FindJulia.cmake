# - Try to find Julia
#
# To provide the module with a hint about where to find your Julia
# installation, you can set the environment variable JULIA_ROOT. The Find
# module will then look in this path when searching for Julia paths and
# libraries.
#
# Once done this will define
#  JULIA_FOUND - System has Julia
#  JULIA_INCLUDE_DIRS - The Julia include directories
#  JULIA_LIBRARIES - The libraries needed to use Julia

if( JULIA_INCLUDE_DIRS AND JULIA_LIBRARIES )
  # Already in cache, be silent
  set( JULIA_FIND_QUIETLY TRUE )
endif( JULIA_INCLUDE_DIRS AND JULIA_LIBRARIES )

find_path( JULIA_INCLUDE_DIR julia.h
  PATHS /usr /usr/local
  HINTS ${JULIA_ROOT}
    ENV JULIA_ROOT
  PATH_SUFFIXES include/julia
  #NO_DEFAULT_PATH
  #NO_SYSTEM_ENVIRONMENT_PATH
  )

find_library( JULIA_LIBRARY NAMES julia
  PATHS /usr /usr/local
  HINTS ${JULIA_ROOT}
    ENV JULIA_ROOT
  PATH_SUFFIXES lib/julia lib64/julia
  #NO_DEFAULT_PATH
  #NO_SYSTEM_ENVIRONMENT_PATH
  )

set( JULIA_LIBRARIES ${JULIA_LIBRARY} )
set( JULIA_INCLUDE_DIRS ${JULIA_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Julia DEFAULT_MSG JULIA_LIBRARY JULIA_INCLUDE_DIR )

mark_as_advanced( JULIA_LIBRARIES JULIA_INCLUDE_DIRS )
