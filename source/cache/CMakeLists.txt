cmake_minimum_required( VERSION 2.8 )

find_package( LibMemcached )
find_package( OpenSSL REQUIRED )

set( LIB_NAME idamcache )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -std=gnu99" )
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )

option( DEBUG "Debug info" ON )
if( DEBUG )
  add_definitions( -DDEBUG )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0" )
endif( DEBUG )

set( SRC_FILES
  idamCache.c
  idamFileCache.c
)

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/client
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/server
  ${SRC_DIR}/logging
  ${SRC_DIR}/structures
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

add_library( ${LIB_NAME} OBJECT ${SRC_FILES} )

if( LIBMEMCACHED_FOUND )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
endif()
