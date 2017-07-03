# - Try to find libppf
#
# To provide the module with a hint about where to find your PPF
# installation, you can set the environment variable PPF_ROOT. The Find
# module will then look in this path when searching for PPF paths and
# libraries.
#
# Once done this will define
#  LIBPPF_FOUND - System has Libppf
#  LIBPPF_INCLUDE_DIRS - The Libppf include directories
#  LIBPPF_LIBRARIES - The libraries needed to use Libppf

if( LIBPPF_INCLUDE_DIRS AND LIBPPF_LIBRARIES )
  # Already in cache, be silent
  set( LIBPPF_FIND_QUIETLY TRUE )
endif( LIBPPF_INCLUDE_DIRS AND LIBPPF_LIBRARIES )

find_path( LIBPPF_INCLUDE_DIR ppf.h
  PATHS /jet/share/include
  HINTS ${PPF_ROOT}
    ENV PPF_ROOT
  PATH_SUFFIXES include
)

find_library( LIBPPF_LIBRARY NAMES ppf libppf
  PATHS /jet/share/lib
  HINTS ${PPF_ROOT}
    ENV PPF_ROOT
  PATH_SUFFIXES lib lib64
)

find_library( LIBNETCSL_LIBRARY NAMES netcsl7 libnetcsl7
  PATHS /jet/share/lib
  HINTS ${PPF_ROOT}
    ENV PPF_ROOT
  PATH_SUFFIXES lib lib64
)

set( LIBPPF_LIBRARIES ${LIBPPF_LIBRARY} ${LIBNETCSL_LIBRARY} )
set( LIBPPF_INCLUDE_DIRS ${LIBPPF_INCLUDE_DIR} )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( PPF DEFAULT_MSG LIBPPF_LIBRARY LIBNETCSL_LIBRARY LIBPPF_INCLUDE_DIR )

mark_as_advanced( LIBPPF_LIBRARIES LIBPPF_INCLUDE_DIRS )
