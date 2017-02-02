cmake_minimum_required( VERSION 3.0 )

add_subdirectory( cache )
add_subdirectory( logging )
add_subdirectory( security )
add_subdirectory( structures )
add_subdirectory( clientserver )
add_subdirectory( client )

if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

if( FAT_BUILD OR NOT CLIENT_ONLY )
  add_subdirectory( server )
  add_subdirectory( plugins )
endif()
