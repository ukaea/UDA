# - Try to find LibTS
# Once done this will define
#
#  LIBTS_FOUND - system has LibTS
#  LIBTS_INCLUDE_DIRS - the LibTS include directory
#  LIBTS_LIBRARIES - Link these to use LibTS
#  LIBTS_DEFINITIONS - Compiler switches required for using LibTS
#

if( LIBTS_LIBRARIES AND LIBTS_INCLUDE_DIRS )
  # Already in cache, be silent
  set( LIBTS_FIND_QUIETLY TRUE )
endif( LIBTS_LIBRARIES AND LIBTS_INCLUDE_DIRS )

find_path( LIBTS_INCLUDE_DIRS tsdef.h
  HINTS ${LIBTS_ROOT}
    ENV LIBTS_DIR
  PATHS
    /usr/local
    /opt/local
    /sw
    /usr/lib/sfw
  PATH_SUFFIXES include )

find_library( LIBTS_LIBRARIES NAMES TS
  HINTS ${LIBTS_ROOT}
    ENV LIBTS_DIR
  PATHS
    /opt/local
    /sw
    /usr
    /usr/local
  PATH_SUFFIXES lib lib64 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LIBTS DEFAULT_MSG LIBTS_LIBRARIES LIBTS_INCLUDE_DIRS )

# show the LIBTS_INCLUDE_DIRS and LIBTS_LIBRARIES variables only in the advanced view
mark_as_advanced( LIBTS_INCLUDE_DIRS LIBTS_LIBRARIES )
