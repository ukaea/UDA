if( NOT CLIENT_ONLY )
  include( LibXml2StrPrintfConfig )
endif()
set( CMAKE_PREFIX_PATH ${_VCPKG_INSTALLED_DIR}/x64-mingw-static )
find_package( Boost 1.60 REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )

# Hack to build on GCC/12.2.0
if( NOT WIN32 OR MINGW )
  if ( NOT APPLE )
    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 11.0 )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-use-after-free -Wno-mismatched-new-delete" )
    endif()
  endif()
endif()

find_package( LibXml2 REQUIRED )

if( WIN32 AND NOT MINGW )
  add_compile_definitions( MINGW )
  add_compile_options( /W2 /WX )
else()
  add_compile_options( -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers )
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 5.0 )
    add_compile_options( -Wno-format-truncation -Wno-format-overflow )
    if( NOT MINGW AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 11.0 )
      add_compile_options( -Wno-use-after-free )
    endif()
  endif()
endif()

if( NOT WIN32 AND NOT MINGW )
  find_package( TIRPC QUIET )
endif()

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/include )

add_subdirectory( bin )
add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( authentication )
add_subdirectory( structures )
add_subdirectory( clientserver )
add_subdirectory( c_api )
add_subdirectory( client )
add_subdirectory( client2 )
if( ENABLE_CAPNP )
  add_compile_definitions( CAPNP_ENABLED=1 )
  add_subdirectory( serialisation )
endif()

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( NOT CLIENT_ONLY )
  add_subdirectory( server )
  add_subdirectory( server2 )
  add_subdirectory( plugins )
endif()

########################################################################################################################
# Configure files

string( TIMESTAMP BUILD_TIMESTAMP UTC )
configure_file(
  ${CMAKE_SOURCE_DIR}/source/version.h.in
  ${CMAKE_BINARY_DIR}/source/version.h
  @ONLY
)

set( HEADERS
  include/accAPI.h
  include/accessors.h
  include/client.h
  include/clientAPI.h
  include/clientMDS.h
  include/export.h
  include/genStructs.h
  include/initStructs.h
  include/legacy.h
  include/pluginStructs.h
  include/struct.h
  include/udaDefines.h
  include/udaErrors.h
  include/udaGetAPI.h
  include/udaPlugin.h
  include/udaPutAPI.h
  include/udaStructs.h
  include/udaTypes.h
)
install( FILES uda.h ${CMAKE_BINARY_DIR}/source/version.h DESTINATION include/uda )
install( FILES ${HEADERS} DESTINATION include/uda )

set( ENV_PATH )
if( WIN32 AND NOT MINGW )
  set( ENV_PATH "set" )
else()
  execute_process( COMMAND which env OUTPUT_VARIABLE ENV_PATH )
endif()
string( STRIP "${ENV_PATH}" ENV_PATH )

find_program( PYTHON_EXE NAMES python python3 PATHS /usr/bin/ )

configure_file(
  ${CMAKE_SOURCE_DIR}/source/bin/install_plugin.in
  ${CMAKE_BINARY_DIR}/bin/install_plugin
  @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/bin/install_plugin
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

set( PKGCONFIG_FILES
  uda-client.pc
  uda-fat-client.pc
  uda-cpp.pc
  uda-fat-cpp.pc
  uda-plugins.pc
)

get_filename_component( LIBXML_LIB_DIR "${LIBXML2_LIBRARIES}" DIRECTORY )
set( PKGCONFIG_LIBRARIES "-L${LIBXML_LIB_DIR} -lxml2" )
set( PKGCONFIG_INCLUDES "-I${LIBXML2_INCLUDE_DIR}" )
set( PKGCONFIG_REQUIRES fmt )

find_package( TIRPC QUIET )
if( TIRPC_FOUND )
    set( PKGCONFIG_REQUIRES "${PKGCONFIG_REQUIRES} libtirpc" )
endif()

if( ENABLE_CAPNP )
  set( PKGCONFIG_REQUIRES "${PKGCONFIG_REQUIRES} capnp" )
  set( PKGCONFIG_LIBRARIES "${PKGCONFIG_LIBRARIES} -luda_serialisation" )
endif()

find_package( LibMemcached QUIET )

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( PKGCONFIG_LIBRARIES "${PKGCONFIG_LIBRARIES} ${LIBMEMCACHED_LIBRARIES}" )
endif()

if( TIRPC_FOUND )
  set( PKGCONFIG_REQUIRES "${PKGCONFIG_REQUIRES} libtirpc" )
endif()

foreach( PKGCONFIG_FILE ${PKGCONFIG_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${PKGCONFIG_FILE}.in"
    "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PKGCONFIG_FILE}"
    @ONLY
  )
endforeach()

find_program( XINETD_EXE xinetd PATHS /usr/sbin/ /usr/local/sbin/ )

configure_file(
  ${CMAKE_SOURCE_DIR}/source/etc/rc.uda.in
  ${CMAKE_BINARY_DIR}/etc/rc.uda
  @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/etc/rc.uda
  DESTINATION etc
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
