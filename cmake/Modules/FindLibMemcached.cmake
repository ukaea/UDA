# - Find LibMemcached
#
# To provide the module with a hint about where to find your LibMemcached
# installation, you can set the environment variables LIBMEMCACHED_DIR. The
# Find module will then look in this path when searching for LibMemcached paths
# and libraries.
#
# Find the LibMemcached includes and libraries
#
#  LIBMEMCACHED_INCLUDES        - Where to find memcached.h, etc
#  LIBMEMCACHED_LIBRARY_DIRS    - Where to find the libraries
#  LIBMEMCACHED_LIBRARIES       - Link these libraries when using LibMemcached
#  LIBMEMCACHED_FOUND           - True if LibMemcached found
#
# Normal usage would be:
#  find_package (LibMemcached REQUIRED)
#  target_link_libraries (name ${LIBMEMCACHED_LIBRARIES})

# Check using pkg-config first
find_package( PkgConfig QUIET )
if( PkgConfig_FOUND )
  pkg_check_modules( LIBMEMCACHED QUIET libmemcached )
endif()

if( LibMemcached_FIND_QUIETLY )
  find_package( Threads QUIET )
elseif( LibMemcached_FIND_REQUIRED )
  find_package( Threads REQUIRED )
else()
  find_package( Threads )
endif()

find_path( LIBMEMCACHED_INCLUDES libmemcached/memcached.h
  HINTS
  ${LIBMEMCACHED_ROOT}
  ${LIBMEMCACHED_INCLUDE_DIRS}
  ENV LIBMEMCACHED_ROOT
  PATH_SUFFIXES include )

find_library( LIBMEMCACHED_LIB NAMES memcached
  HINTS
  ${LIBMEMCACHED_ROOT}
  ${LIBMEMCACHED_LIBRARY_DIRS}
  ENV LIBMEMCACHED_ROOT
  PATH_SUFFIXES lib lib64 )

set( LIBMEMCACHED_LIBRARIES ${LIBMEMCACHED_LIB} ${CMAKE_THREAD_LIBS_INIT} )
get_filename_component( LIBMEMCACHED_LIB_DIR "${LIBMEMCACHED_LIB}" PATH CACHE )

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( LibMemcached DEFAULT_MSG LIBMEMCACHED_LIB LIBMEMCACHED_INCLUDES Threads_FOUND )

mark_as_advanced( LIBMEMCACHED_LIBRARIES LIBMEMCACHED_INCLUDES )

if( LIBMEMCACHED_FOUND AND NOT TARGET LibMemcached::LibMemcached )
  add_library( LibMemcached::LibMemcached UNKNOWN IMPORTED )
  set_target_properties( LibMemcached::LibMemcached PROPERTIES
    IMPORTED_LOCATION "${LIBMEMCACHED_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBMEMCACHED_INCLUDES}"
    INTERFACE_LINK_LIBRARIES "Threads::Threads"
  )
endif()
