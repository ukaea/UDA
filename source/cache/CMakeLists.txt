find_package( LibMemcached )

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
  cache.cpp
  fileCache.cpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif(TIRPC_FOUND)
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

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
