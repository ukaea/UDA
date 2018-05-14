# - Try to find LibSSH
# Once done this will define
#
#  LIBSSH_FOUND - system has LibSSH
#  LIBSSH_INCLUDE_DIRS - the LibSSH include directory
#  LIBSSH_LIBRARIES - Link these to use LibSSH
#  LIBSSH_DEFINITIONS - Compiler switches required for using LibSSH
#

if( LIBSSH_LIBRARIES AND LIBSSH_INCLUDE_DIRS )
  # Already in cache, be silent
  set( LIBSSH_FIND_QUIETLY TRUE )
endif( LIBSSH_LIBRARIES AND LIBSSH_INCLUDE_DIRS )

find_path( LIBSSH_INCLUDE_DIRS libssh/libssh.h
  HINTS ${LIBSSH_ROOT}
    ENV LIBSSH_DIR
  PATHS
    /usr/local
    /opt/local
    /sw
    /usr/lib/sfw
  PATH_SUFFIXES include )

find_library( LIBSSH_LIBRARIES NAMES ssh
  HINTS ${LIBSSH_ROOT}
    ENV LIBSSH_DIR
  PATHS
    /opt/local
    /sw
    /usr
    /usr/local
  PATH_SUFFIXES lib lib64 )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LIBSSH DEFAULT_MSG LIBSSH_LIBRARIES LIBSSH_INCLUDE_DIRS )

# show the LIBSSH_INCLUDE_DIRS and LIBSSH_LIBRARIES variables only in the advanced view
mark_as_advanced( LIBSSH_INCLUDE_DIRS LIBSSH_LIBRARIES )
