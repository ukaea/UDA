find_package( Boost COMPONENTS program_options REQUIRED )

option( UDA_CLI_BOOST_STATIC "compile commandline interface with static boost libraries" OFF )

if( UDA_CLI_BOOST_STATIC )
  set( Boost_USE_STATIC_LIBS ON )
endif()

add_executable( uda_cli uda_cli.cpp )
target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )

target_compile_definitions( uda_cli PRIVATE CAPNP_ENABLED=1 )

target_link_libraries( uda_cli PRIVATE uda_cpp client serialisation client_server structures cache common config logging )
target_link_libraries( uda_cli PRIVATE Boost::program_options )

install(
  TARGETS uda_cli
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)