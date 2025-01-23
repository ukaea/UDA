find_package( fmt REQUIRED )

set( LIBS fmt::fmt )

if( UDA_CACHE_MEMCACHE )
  find_package( LibMemcached REQUIRED )
  list( APPEND LIBS LibMemcached::memcached )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

set( SRCS
  memcache.cpp
  fileCache.cpp
  cache.cpp
)

add_library( cache OBJECT ${SRCS} )
target_include_directories( cache PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( cache PRIVATE fmt::fmt )