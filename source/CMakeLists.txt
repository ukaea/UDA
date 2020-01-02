include( LibXml2StrPrintfConfig )

if( WIN32 AND NOT MINGW )
  add_definitions( /W2 /WX )
else()
  add_definitions( -Wall -Werror -Wextra -Wno-unused-parameter -Wno-missing-field-initializers )
  if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 5.0 )
    add_definitions( -Wno-format-truncation -Wno-format-overflow )
  endif()
endif()

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( authentication )
add_subdirectory( structures )
#add_subdirectory( config )
add_subdirectory( clientserver )
add_subdirectory( client )

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( NOT CLIENT_ONLY )
  add_subdirectory( server )
  add_subdirectory( plugins )
endif()

install( FILES uda.h DESTINATION include/uda )

execute_process( COMMAND which env OUTPUT_VARIABLE ENV_PATH )
string( STRIP ${ENV_PATH} ENV_PATH )

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