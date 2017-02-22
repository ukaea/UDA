# - Try to find KSBA
# Once done this will define
#
#  KSBA_FOUND - system has KSBA
#  KSBA_INCLUDE_DIRS - the KSBA include directory
#  KSBA_LIBRARIES - Link these to use KSBA
#  KSBA_DEFINITIONS - Compiler switches required for using KSBA
#

if( KSBA_LIBRARIES AND KSBA_INCLUDE_DIRS )
  # in cache already
  # set(KSBA_FOUND TRUE)
else( KSBA_LIBRARIES AND KSBA_INCLUDE_DIRS )

  set( _KSBA_ROOT_PATHS
    "$ENV{PROGRAMFILES}/libKSBA"
    )

  find_path( KSBA_ROOT_DIR
    NAMES
      include/ksba.h
    PATHS
      ${_KSBA_ROOT_PATHS}
    )
  mark_as_advanced( KSBA_ROOT_DIR )

  find_path( KSBA_INCLUDE_DIR
    NAMES
      ksba.h
    PATHS
      /usr/local/include
      /opt/local/include
      /sw/include
      /usr/lib/sfw/include
      ${KSBA_ROOT_DIR}/include
    )
  set( KSBA_INCLUDE_DIRS ${KSBA_INCLUDE_DIR} )

  find_library( KSBA_LIBRARY
    NAMES
      ksba
    PATHS
      /opt/local/lib
      /sw/lib
      /usr/lib
      /usr/local/lib
      ${KSBA_ROOT_DIR}/lib
    )

  find_library( GPG_ERROR_LIBRARY
    NAMES
      gpg-error
    PATHS
      /opt/local/lib
      /sw/lib
      /usr/lib
      /usr/local/lib
      ${KSBA_ROOT_DIR}/lib
    )

  set( KSBA_LIBRARIES ${KSBA_LIBRARY} ${GPG_ERROR_LIBRARY} )

  include( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( KSBA DEFAULT_MSG KSBA_LIBRARIES KSBA_INCLUDE_DIRS )

  # show the KSBA_INCLUDE_DIRS and KSBA_LIBRARIES variables only in the advanced view
  mark_as_advanced( KSBA_INCLUDE_DIRS KSBA_LIBRARIES )

endif( KSBA_LIBRARIES AND KSBA_INCLUDE_DIRS )