add_subdirectory( authentication )
add_subdirectory( bin )
add_subdirectory( cache )
add_subdirectory( client )
add_subdirectory( client2 )
add_subdirectory( clientserver )
add_subdirectory( logging )
if( NOT CLIENT_ONLY )
  add_subdirectory( plugins )
endif()
add_subdirectory( security )
if( ENABLE_CAPNP )
  add_subdirectory( serialisation )
endif()
if( NOT CLIENT_ONLY )
  add_subdirectory( server )
  add_subdirectory( server2 )
endif()
add_subdirectory( structures )
if( NOT NO_WRAPPERS )
  add_subdirectory( wrappers )
endif()

########################################################################################################################
# Configure files

string( TIMESTAMP BUILD_TIMESTAMP UTC )
configure_file(
  ${CMAKE_SOURCE_DIR}/source/version.h.in
  ${CMAKE_BINARY_DIR}/include/uda/version.h
  @ONLY
)

install( FILES uda.h ${CMAKE_BINARY_DIR}/source/version.h DESTINATION include/uda )