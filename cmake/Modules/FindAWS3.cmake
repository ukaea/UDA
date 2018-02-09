# - Try to find AWS S3 Installation

# To provide the module with a hint about where to find the AWS S3
# installation, you can set the environment variable AWS_ROOT. The Find
# module will then look in this path when searching for paths and
# libraries.
#
# Once done this will define
#  LIBAWS3_FOUND - System has libaws*
#  LIBAWS3_INCLUDE_DIRS - The AWS S3 include directories
#  LIBAWS3_LIBRARIES - The libraries needed 

if( LIBAWS3_INCLUDE_DIRS AND LIBAWS3_LIBRARIES )
  # Already in cache, be silent
  set( LIBAWS3_FIND_QUIETLY TRUE )
endif( LIBAWS3_INCLUDE_DIRS AND LIBAWS3_LIBRARIES )

find_path( LIBAWS3_S3_INCLUDE_DIR aws/s3/S3Client.h
  PATHS /usr/local 
  HINTS ${AWS3_ROOT}
    ENV AWS3_ROOT
  PATH_SUFFIXES include
)

find_path( LIBAWS3_CPP_INCLUDE_DIR1 cstdlib
  PATHS /usr/include
  HINTS ${AWS3_ROOT}
    ENV AWS3_ROOT
  PATH_SUFFIXES c++/4.8.2
)
set( LIBAWS3_CPP_INCLUDE_DIR2
  /usr/include/c++/4.8.2/x86_64-redhat-linux
)


find_library( LIBAWS3_S3_LIBRARY NAMES aws-cpp-sdk-s3
  PATHS /usr/local  
  HINTS ${AWS3_ROOT}
    ENV AWS3_ROOT
  PATH_SUFFIXES lib64 
)

find_library( LIBAWS3_SDK_LIBRARY NAMES aws-cpp-sdk-core
  PATHS /usr/local  
  HINTS ${AWS3_ROOT}
    ENV AWS3_ROOT
  PATH_SUFFIXES lib64
)

set( LIBAWS3_LIBRARIES ${LIBAWS3_S3_LIBRARY} ${LIBAWS3_SDK_LIBRARY} )
set( LIBAWS3_INCLUDE_DIRS ${LIBAWS3_S3_INCLUDE_DIR} ${LIBAWS3_CPP_INCLUDE_DIR1} ${LIBAWS3_CPP_INCLUDE_DIR2})

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LIBAWS3 DEFAULT_MSG LIBAWS3_LIBRARIES LIBAWS3_INCLUDE_DIRS )

mark_as_advanced( LIBAWS3_LIBRARIES LIBAWS3_INCLUDE_DIRS )
