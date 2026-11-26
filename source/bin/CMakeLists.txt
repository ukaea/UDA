if( "${NO_CLI}" )
    message( WARNING "Not building uda CLI due to option NO_CLI" )
    return()
endif()

add_executable( uda_cli uda_cli.cpp )

option( UDA_CLI_BOOST_STATIC "compile commandline interface with static boost libraries" OFF )

if ( UDA_CLI_BOOST_STATIC )
    set( Boost_USE_STATIC_LIBS ON )
endif()

find_package( Boost COMPONENTS program_options REQUIRED )

target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )

target_link_libraries( uda_cli PRIVATE client-static uda_cpp-static )
target_link_libraries( uda_cli PRIVATE Boost::program_options )

if( ENABLE_CAPNP )
  find_package( CapnProto REQUIRED )

  target_link_libraries( uda_cli PRIVATE serialisation-static )
  target_compile_definitions( uda_cli PRIVATE CAPNP_ENABLED=1 )
  
  find_library( STDCXX_LIBRARY_FILE stdc++ HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} )
  get_filename_component(STDCXX_LIBRARY_DIR ${STDCXX_LIBRARY_FILE} DIRECTORY )
  
  # Directories that the system already searches are not added to RUNPATH; packages must not
  # carry build-machine library paths.
  set( _LINK_FLAGS )
  if ( NOT "${STDCXX_LIBRARY_DIR}" STREQUAL "" AND NOT "${STDCXX_LIBRARY_DIR}" MATCHES "^/(usr/)?lib" )
    string( APPEND _LINK_FLAGS " -Wl,-rpath,${STDCXX_LIBRARY_DIR} -L${STDCXX_LIBRARY_DIR}" )
  endif()
  if ( NOT "${capnp_LIBRARY_DIRS}" STREQUAL "" )
    if ( NOT "${capnp_LIBRARY_DIRS}" MATCHES "^/(usr/)?lib" )
      string( APPEND _LINK_FLAGS " -Wl,-rpath,${capnp_LIBRARY_DIRS} -L${capnp_LIBRARY_DIRS}" )
    endif()
    string( APPEND _LINK_FLAGS " -lcapnp -lkj" )
  endif()
  set_target_properties( uda_cli PROPERTIES LINK_FLAGS "${_LINK_FLAGS}" )
endif()

install(
  TARGETS uda_cli
  DESTINATION ${UDA_INSTALL_BINDIR}
  COMPONENT tools
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

# A convenience script: it has a shebang, so it is installed executable.
install(
  PROGRAMS log_parser.py
  DESTINATION ${UDA_INSTALL_TOOLSDIR}
  COMPONENT tools
)
