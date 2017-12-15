# - Try to find LIBS3 S3 API Installation

# To provide the module with a hint about where to find the LIBS3
# installation, you can set the environment variable LIBS3_ROOT. The Find
# module will then look in this path when searching for paths and
# libraries.
#
# Once done this will define
#  LIBS3_FOUND - System has libs3.so
#  LIBS3_INCLUDE_DIRS - The LIBS3 include directories
#  LIBS3_LIBRARIES - The libraries needed 

if( LIBS3_INCLUDE_DIRS AND LIBS3_LIBRARIES )
  # Already in cache, be silent
  set( LIBS3_FIND_QUIETLY TRUE )
endif( LIBS3_INCLUDE_DIRS AND LIBS3_LIBRARIES )

find_path( LIBS3_INCLUDE_DIR libs3.h
  PATHS /usr/local 
  HINTS ${LIBS3_ROOT}
    ENV LIBS3_ROOT
  PATH_SUFFIXES include
)

find_library( LIBS3_LIBRARY NAMES s3
  PATHS /usr/local  
  HINTS ${LIBS3_ROOT}
    ENV LIBS3_ROOT
  PATH_SUFFIXES
    lib
    lib64 
)

set( LIBS3_LIBRARIES ${LIBS3_LIBRARY} )
set( LIBS3_INCLUDE_DIRS ${LIBS3_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LIBS3 DEFAULT_MSG LIBS3_LIBRARIES LIBS3_INCLUDE_DIRS )

mark_as_advanced( LIBS3_LIBRARIES LIBS3_INCLUDE_DIRS )
