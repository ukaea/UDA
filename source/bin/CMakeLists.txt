add_executable( uda_cli uda_cli.cpp )

set( Boost_USE_STATIC_LIBS ON )
find_package( Boost COMPONENTS program_options REQUIRED )

target_include_directories( uda_cli PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/extlib/GSL-4.0.0/include )

target_link_libraries( uda_cli PRIVATE client-static uda_cpp-static )
target_link_libraries( uda_cli PRIVATE Boost::program_options )

if( ENABLE_CAPNP )
  find_package( CapnProto REQUIRED )

  target_link_libraries( uda_cli PRIVATE  serialisation-static )
  
  find_library( STDCXX_LIBRARY_FILE stdc++ HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} )
  get_filename_component(STDCXX_LIBRARY_DIR ${STDCXX_LIBRARY_FILE} DIRECTORY )
  
  if ( "${STDCXX_LIBRARY_FILE}" STREQUAL "" )
    set ( STDCXX_RPATH "" ) 
  else()
      set ( STDCXX_RPATH "-Wl,-rpath,${STDCXX_LIBRARY_FILE} -L${STDCXX_LIBRARY_FILE} -lstdc++" ) 
      
  endif()

  # get_filename_component( CAPNP_LIB_CMAKE_DIR ${CapnProto_DIR} DIRECTORY )
  # get_filename_component( CAPNP_LIB_DIR ${CAPNP_LIB_CMAKE_DIR} DIRECTORY )

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
