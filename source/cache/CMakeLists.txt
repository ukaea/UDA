find_package( LibMemcached QUIET )
find_package( Boost REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )

set( LIBS Boost::boost OpenSSL::SSL LibXml2::LibXml2 fmt::fmt )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
    list( APPEND LIBS dlfcn-win32::dl )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

set( SRC_FILES
  memcache.cpp
  fileCache.cpp
  cache.cpp
)

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  list( APPEND LIBS LibMemcached::memcached )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

add_library( cache-objects OBJECT ${SRC_FILES} )
target_link_libraries( cache-objects PRIVATE ${LIBS} )
target_include_directories( cache-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( MINGW )
  target_compile_definitions( cache-objects PRIVATE -Wno-format-truncation -Wno-format-overflow )
endif()
