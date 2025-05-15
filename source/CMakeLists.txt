if( NOT CLIENT_ONLY )
  include( LibXml2StrPrintfConfig )
endif()
set( CMAKE_PREFIX_PATH ${_VCPKG_INSTALLED_DIR}/x64-mingw-static )
find_package( Boost 1.60 REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )

# Hack to build on GCC/12.2.0
if( NOT WIN32 OR MINGW )
  if ( NOT APPLE AND NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM") )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-use-after-free -Wno-mismatched-new-delete" )
  endif()
endif()

find_package( LibXml2 REQUIRED )

if( WIN32 AND NOT MINGW )
  add_compile_definitions( MINGW )
  message( AUTHOR_WARNING "BEWARE: Not treating MSVC warnings as errors due to known issue compiling capnp schema" )
  add_definitions( /W2 )
  #add_definitions( /W2 /WX )
else()
  add_definitions( -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers )
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 5.0 )
    add_definitions( -Wno-format-truncation -Wno-format-overflow )
    if( NOT MINGW AND NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "IntelLLVM") )
      add_definitions( -Wno-use-after-free )
    endif()
  endif()
  if( APPLE )
    add_definitions( -Wno-cast-function-type )
  endif()
endif()

if( NOT WIN32 AND NOT MINGW )
  find_package( TIRPC QUIET )
endif()

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( bin )
add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( authentication )
add_subdirectory( structures )
add_subdirectory( clientserver )
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

install( FILES uda.h ${CMAKE_BINARY_DIR}/source/version.h DESTINATION include/uda )

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

install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/lib/pkgconfig
  DESTINATION lib
)

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

set( SERVICE_FILES
  uda.socket
  uda@.service
)

foreach( SERVICE_FILE ${SERVICE_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${SERVICE_FILE}.in"
    "${CMAKE_BINARY_DIR}/etc/${SERVICE_FILE}"
    @ONLY
  )

  install(
    FILES "${CMAKE_BINARY_DIR}/etc/${SERVICE_FILE}"
    DESTINATION etc
    PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
  )
endforeach()

