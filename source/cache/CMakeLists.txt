########################################################################################################################
# Dependencies

find_package( LibMemcached QUIET )
find_package( fmt REQUIRED )

########################################################################################################################
# Targets

add_library( cache-objects OBJECT )
add_library( uda::cache-objects ALIAS cache-objects )

target_sources( cache-objects PRIVATE memcache.cpp fileCache.cpp cache.cpp )
target_compile_features( cache-objects PRIVATE cxx_std_11 )
target_include_directories( cache-objects PRIVATE ${PROJECT_SOURCE_DIR}/source )
target_link_libraries( cache-objects PUBLIC fmt::fmt )

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  target_link_libraries( cache-objects PUBLIC LibMemcached::libmemcached )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
  set( CACHE_LIBRARIES ${LIBMEMCACHED_LIBRARIES} PARENT_SCOPE )
else()
  message( WARNING "Not compiling with memcached support" )
  target_compile_definitions( cache-objects PRIVATE -DNOLIBMEMCACHED )
endif()

if( MINGW )
  target_compile_definitions( cache-objects PRIVATE -Wno-format-truncation -Wno-format-overflow )
endif()
