add_executable( uda_cli uda_cli.cpp )

find_package( Boost COMPONENTS program_options REQUIRED )

target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )

if( BUILD_SHARED_LIBS )
  target_link_libraries( uda_cli PRIVATE client-shared uda_cpp-shared )
else()
  target_link_libraries( uda_cli PRIVATE client-static uda_cpp-static )
endif()
if( ENABLE_CAPNP )
  target_link_libraries( uda_cli PRIVATE serialisation-static )
endif()

target_link_libraries( uda_cli PRIVATE Boost::program_options )

install(
  TARGETS uda_cli
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
