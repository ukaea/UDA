include( FetchContent )

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable( Catch2 )

set( PLUGIN_TESTS
  testplugin
)

include( CaseConvertors )

file( REAL_PATH test_config.toml CONFIG_FILE_PATH )
configure_file( test_config.hpp.in test_config.hpp @ONLY )

function( build_test name )
  if( TARGET ${name}_plugin )
    message( "Target ${name}_plugin exists -> building test for ${name}" )
  else()
    message( "Target ${name}_plugin does not exist -> skipping test for ${name}" )
    return()
  endif()

  set( target "test_${name}" )
  set( source "test_${name}.cpp" )

  snake_to_proper_case( ${name} proper_name )
  set( test_name "Test${proper_name}" )

  add_executable( ${target} ${source} )
  target_include_directories( ${target} PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_BINARY_DIR} )
  target_link_libraries( ${target} PRIVATE Catch2::Catch2WithMain c_api )
  if( ENABLE_CAPNP )
    target_link_libraries( ${target} PRIVATE "$<BUILD_INTERFACE:serialisation>" )
    target_compile_definitions( ${target} PRIVATE -DCAPNP_ENABLED=1 )
  endif()

  add_test( NAME ${test_name} COMMAND ${target} CONFIGURATIONS server )
endfunction(  )

foreach( PLUGIN_NAME ${PLUGIN_TESTS} )
  build_test( ${PLUGIN_NAME} )
endforeach()