cmake_minimum_required( VERSION 3.0 )

include( LibXml2StrPrintfConfig )

add_definitions(
  -Wall
  -Wno-error
)

set( LOGGING_BACKEND "FILE" CACHE STRING "Choose logging backend [FILE;SYSLOG]" )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( security )
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
