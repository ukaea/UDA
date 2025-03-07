Include( FetchContent )

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0 # or a later release
)

FetchContent_MakeAvailable( Catch2 )

function( snake_to_proper_case input out_var )
  string( REPLACE "_" ";" tokens "${input}" )
  set( ${out_var} "" )
  foreach( token ${tokens} )
    string( SUBSTRING ${token} 0 1 first_char )
    string( SUBSTRING ${token} 1 -1 remainder )
    string( TOUPPER ${first_char} upper_first_char )
    string( TOLOWER ${remainder} lower_remainder )
    set( new_token "${upper_first_char}${lower_remainder}" )
    string( APPEND ${out_var} ${new_token} )
  endforeach(  )
  return( PROPAGATE ${out_var} )
endfunction( snake_to_proper_case )

function( define_test name )
  set( options SERIAL )
  cmake_parse_arguments( PARSE_ARGV 0 arg "${options}" "" "" )

  set( file "${name}.test.cpp" )
  set( target "test_${name}" )
  snake_to_proper_case( ${name} proper_name )
  set( test_name "Test${proper_name}" )

  add_executable( ${target} ${file} )
  target_include_directories( ${target} PRIVATE ${CMAKE_SOURCE_DIR}/source ${CMAKE_SOURCE_DIR}/include )
  target_link_libraries( ${target} PRIVATE Catch2::Catch2WithMain c_api )
  add_test( NAME ${test_name} COMMAND ${target} )
  if ( ${arg_SERIAL} )
    set_tests_properties( ${test_name} PROPERTIES RUN_SERIAL TRUE )
  endif(  )
endfunction( define_test )

add_subdirectory( client )
add_subdirectory( clientserver )

