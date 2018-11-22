cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( LibMemcached )

# Format overflow inside sprintf
if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.0 )
  add_definitions( -Wno-error=format-overflow )
endif()

set( SRC_FILES
  cache.c
  fileCache.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

if( LIBMEMCACHED_FOUND )
  include_directories( ${LIBMEMCACHED_INCLUDES} )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

add_library( cache-objects OBJECT ${SRC_FILES} )

if( LIBMEMCACHED_FOUND )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
endif()
