find_package( LibMemcached QUIET )
find_package( Boost REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
if( FETCH_FMT )
  FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(fmt)
  set (fmt_DIR ${fmt_BINARY_DIR} )
endif()
find_package( fmt REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions(-D__TIRPC__)
  endif()
endif()

set( SRC_FILES
  memcache.cpp
  fileCache.cpp
  cache.cpp
)

include_directories(
  ${OPENSSL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/source
  ${LIBXML2_INCLUDE_DIR}
  ${FMT_INCLUDE_DIR}
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif(TIRPC_FOUND)
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  include_directories( ${LIBMEMCACHED_INCLUDES} )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

add_library( cache-objects OBJECT ${SRC_FILES} )
target_link_libraries( cache-objects PRIVATE fmt::fmt )

#if( LIBMEMCACHED_FOUND )
#  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
#endif()

if( MINGW )
  add_definitions( -Wno-format-truncation -Wno-format-overflow )
endif()
