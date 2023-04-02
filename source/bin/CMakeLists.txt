find_package( Boost COMPONENTS program_options REQUIRED )

if( NOT NO_WRAPPERS )
  add_executable( uda_cli uda_cli.cpp )
  target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_BINARY_DIR}/include )

  target_compile_features( uda_cli PRIVATE cxx_std_17 )

  if( BUILD_SHARED_LIBS )
    target_link_libraries( uda_cli PRIVATE uda::client-shared uda::uda_cpp-shared )
  else()
    target_link_libraries( uda_cli PRIVATE uda::client-static uda::uda_cpp-static )
  endif()

  target_link_libraries( uda_cli PRIVATE Boost::program_options )

  install(
    TARGETS uda_cli
    DESTINATION bin
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
  )
endif()

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