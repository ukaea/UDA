cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( LibMemcached )
find_package( OpenSSL REQUIRED )

set( SRC_FILES
  idamCache.c
  idamFileCache.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

if( LIBMEMCACHED_FOUND )
  include_directories( ${LIBMEMCACHED_INCLUDES} ${OPENSSL_INCLUDE_DIR} )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

add_definitions( -DCOMPRESSOFF )
add_definitions( -DOSTYPE=linux )
add_definitions( -DISCOMPLIANT )
add_definitions( -D_GNU_SOURCE )
add_definitions( -DSINGLEPACKET )
add_definitions( -DGENERALSTRUCTS )
add_definitions( -DDGMJUL14 )
add_definitions( -DCLEANNAMESPACE25SEP14 )
add_definitions( -DFILECACHE )

add_library( cache OBJECT ${SRC_FILES} )

if( LIBMEMCACHED_FOUND )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
endif()
