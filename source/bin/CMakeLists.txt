add_executable( uda_cli uda_cli.cpp )

find_package( Boost COMPONENTS program_options REQUIRED )

target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/source )

if( BUILD_SHARED_LIBS )
  target_link_libraries( uda_cli PRIVATE client-shared uda_cpp-shared serialisation-static )
else()
  target_link_libraries( uda_cli PRIVATE client-static uda_cpp-static serialisation-static )
endif()

target_link_libraries( uda_cli PRIVATE Boost::program_options )

install(
  TARGETS uda_cli
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)