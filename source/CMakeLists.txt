cmake_minimum_required( VERSION 3.0 )

include( LibXml2StrPrintfConfig )

if( WIN32 AND NOT MINGW )
  add_definitions( /W2 /WX )
else()
  add_definitions( -Wall -Werror -Wextra -Wno-unused-parameter )
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
install( PROGRAMS bin/install_plugin DESTINATION bin )
