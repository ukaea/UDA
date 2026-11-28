cmake_minimum_required( VERSION 3.12 )
project( uda_pkgconfig_consumer LANGUAGES CXX )

find_package( PkgConfig REQUIRED )

option( UDA_PC_STATIC "Request the static dependency closure from pkg-config (--static)" OFF )

# Returns one kind of flag (--cflags or --libs) for MODULE as a CMake list.
function( uda_pc_flags MODULE KIND OUTVAR )
  set( _ARGS ${KIND} ${MODULE} )
  if( UDA_PC_STATIC )
    list( INSERT _ARGS 0 --static )
  endif()
  execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} ${_ARGS}
    RESULT_VARIABLE _RESULT
    OUTPUT_VARIABLE _OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if( NOT _RESULT EQUAL 0 )
    message( FATAL_ERROR "pkg-config failed for ${MODULE}" )
  endif()
  separate_arguments( _OUTPUT UNIX_COMMAND "${_OUTPUT}" )
  set( ${OUTVAR} "${_OUTPUT}" PARENT_SCOPE )
endfunction()

# Builds SOURCE against MODULE using only the flags pkg-config reports for it.
function( uda_pc_consumer MODULE SOURCE )
  uda_pc_flags( ${MODULE} --cflags CFLAGS )
  uda_pc_flags( ${MODULE} --libs LIBS )

  # Preserve pkg-config's archive order, which matters for static consumers. CMake would treat
  # -L entries as raw linker strings, so they are passed to link_directories() instead.
  set( LINK_LIBS )
  foreach( _FLAG IN LISTS LIBS )
    if( _FLAG MATCHES "^-L(.+)$" )
      link_directories( "${CMAKE_MATCH_1}" )
    else()
      list( APPEND LINK_LIBS "${_FLAG}" )
    endif()
  endforeach()

  string( REPLACE "-" "_" _NAME "pc_${MODULE}" )
  add_executable( ${_NAME} ${SOURCE} )
  target_compile_features( ${_NAME} PRIVATE cxx_std_17 )
  target_compile_options( ${_NAME} PRIVATE ${CFLAGS} )
  target_link_libraries( ${_NAME} PRIVATE ${LINK_LIBS} )
  add_test( NAME ${_NAME} COMMAND ${_NAME} )
endfunction()

enable_testing()
uda_pc_consumer( uda-client ../consumer/main.cpp )
uda_pc_consumer( uda-cpp ../consumer/main_cpp.cpp )
