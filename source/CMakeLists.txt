if (NOT CLIENT_ONLY)
include( LibXml2StrPrintfConfig )
endif()
find_package( Boost REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )

find_package( LibXml2 REQUIRED )


if( WIN32 AND NOT MINGW )
  add_definitions( /W2 /WX )
else()
  add_definitions( -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers )
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 5.0 )
    add_definitions( -Wno-format-truncation -Wno-format-overflow )
  endif()
endif()

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( authentication )
add_subdirectory( structures )
add_subdirectory( clientserver )
add_subdirectory( client )
add_subdirectory( client2 )

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( NOT CLIENT_ONLY )
  add_subdirectory( server )
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
string( STRIP ${ENV_PATH} ENV_PATH )

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
