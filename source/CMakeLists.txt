if( NOT CLIENT_ONLY )
  include( LibXml2StrPrintfConfig )
endif()
list( APPEND CMAKE_PREFIX_PATH ${_VCPKG_INSTALLED_DIR}/x64-mingw-static )
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
    if ( ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL 17 )
      add_definitions( -Wno-cast-function-type-mismatch )
    endif(  )
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

install(
  FILES uda.h ${CMAKE_BINARY_DIR}/source/version.h
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}
  COMPONENT development
)

set( ENV_PATH )
if( WIN32 AND NOT MINGW )
  set( ENV_PATH "set" )
else()
  execute_process( COMMAND which env OUTPUT_VARIABLE ENV_PATH )
endif()
string( STRIP "${ENV_PATH}" ENV_PATH )

configure_file(
  ${CMAKE_SOURCE_DIR}/source/bin/install_plugin.in
  ${CMAKE_BINARY_DIR}/bin/install_plugin
  @ONLY
)

install(
  FILES ${CMAKE_BINARY_DIR}/bin/install_plugin
  DESTINATION ${UDA_INSTALL_BINDIR}
  COMPONENT tools
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

if( NOT CLIENT_ONLY )
  find_program( XINETD_EXE xinetd PATHS /usr/sbin/ /usr/local/sbin/ )

  configure_file(
    ${CMAKE_SOURCE_DIR}/source/etc/rc.uda.in
    ${CMAKE_BINARY_DIR}/etc/rc.uda
    @ONLY
  )

  install(
    PROGRAMS ${CMAKE_BINARY_DIR}/etc/rc.uda
    DESTINATION ${UDA_INSTALL_SERVER_SCRIPTDIR}
    COMPONENT server
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
      DESTINATION ${UDA_INSTALL_SERVICEDIR}
      COMPONENT server
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
  endforeach()

  # The SYSTEM layout logs to /var/log/uda and runs from /run/uda, which nothing else creates.
  # udaserver.sh and uda@.service fail without them, so let systemd-tmpfiles provision them.
  if( UDA_INSTALL_LAYOUT STREQUAL "SYSTEM" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    if( USER )
      set( UDA_TMPFILES_USER "${USER}" )
    else()
      set( UDA_TMPFILES_USER "-" )
    endif()
    configure_file(
      "${CMAKE_SOURCE_DIR}/source/etc/uda-tmpfiles.conf.in"
      "${CMAKE_BINARY_DIR}/etc/uda-tmpfiles.conf"
      @ONLY
    )
    install(
      FILES "${CMAKE_BINARY_DIR}/etc/uda-tmpfiles.conf"
      DESTINATION ${UDA_INSTALL_TMPFILESDIR}
      RENAME uda.conf
      COMPONENT server
      PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )
  endif()
endif()

