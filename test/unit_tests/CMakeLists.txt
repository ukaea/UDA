include( FetchContent )

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable( Catch2 )

include( CaseConvertors )

function( define_test name )
  set( options SERIAL )
  set( oneValueArgs COMPILE_DEFINITIONS )
  cmake_parse_arguments( PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "" )

  set( file "${name}.test.cpp" )
  set( target "test_${name}" )
  snake_to_proper_case( ${name} proper_name )
  set( test_name "Test${proper_name}" )

  add_executable( ${target} ${file} )
  target_include_directories( ${target} PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
  target_link_libraries( ${target} PRIVATE Catch2::Catch2WithMain c_api )
  target_compile_definitions( ${target} PRIVATE ${arg_COMPILE_DEFINITIONS} )
  add_test( NAME ${test_name} COMMAND ${target} CONFIGURATIONS unit )
  if ( ${arg_SERIAL} )
    set_tests_properties( ${test_name} PROPERTIES RUN_SERIAL TRUE )
  endif(  )
endfunction( define_test )

add_subdirectory( client )
add_subdirectory( clientserver )

