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
  
  if ( "${STDCXX_LIBRARY_DIR}" STREQUAL "" )
    set ( STDCXX_RPATH "" ) 
  else()
      set ( STDCXX_RPATH "-Wl,-rpath,${STDCXX_LIBRARY_DIR} -L${STDCXX_LIBRARY_DIR} -lstdc++" )
  endif()

  if ( "${capnp_LIBRARY_DIRS}" STREQUAL "" )
      set( CAPNP_RPATH "" )
  else()
      set( CAPNP_RPATH "-Wl,-rpath,${capnp_LIBRARY_DIRS} -L${capnp_LIBRARY_DIRS} -lcapnp -lkj" )
  endif()

  set_target_properties( uda_cli
    PROPERTIES
    LINK_FLAGS "${CAPNP_RPATH} ${STDCXX_RPATH}" 
  )
endif()

install(
  TARGETS uda_cli
  DESTINATION bin
  PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)
