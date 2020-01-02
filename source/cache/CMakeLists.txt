find_package( LibMemcached )

set( SRC_FILES
  cache.cpp
  fileCache.cpp
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

if( MINGW )
  add_definitions( -Wno-format-truncation -Wno-format-overflow )
endif()
