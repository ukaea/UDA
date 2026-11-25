cmake_minimum_required( VERSION 3.12 )
project( uda_pkgconfig_consumer LANGUAGES CXX )

find_package( PkgConfig REQUIRED )
set( UDA_PC_MODULE "uda-client" CACHE STRING "Installed UDA pkg-config module" )
option( UDA_PC_STATIC "Request the static dependency closure from pkg-config" OFF )

set( _PC_ARGS )
if( UDA_PC_STATIC )
  list( APPEND _PC_ARGS --static )
endif()
foreach( _KIND IN ITEMS cflags libs )
  execute_process(
    COMMAND ${PKG_CONFIG_EXECUTABLE} ${_PC_ARGS} --${_KIND} ${UDA_PC_MODULE}
    RESULT_VARIABLE _PC_RESULT
    OUTPUT_VARIABLE _PC_${_KIND}
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if( NOT _PC_RESULT EQUAL 0 )
    message( FATAL_ERROR "pkg-config failed for ${UDA_PC_MODULE}" )
  endif()
  separate_arguments( _PC_${_KIND} UNIX_COMMAND "${_PC_${_KIND}}" )
endforeach()

# CMake treats raw -L entries in target_link_libraries as linker flag strings.
# Supply directories separately so it can quote prefixes containing spaces.
set( _PC_LINK_LIBS )
foreach( _FLAG IN LISTS _PC_libs )
  if( _FLAG MATCHES "^-L(.+)$" )
    link_directories( "${CMAKE_MATCH_1}" )
  else()
    list( APPEND _PC_LINK_LIBS "${_FLAG}" )
  endif()
endforeach()

if( UDA_PC_MODULE MATCHES "cpp$" )
  set( _SOURCE ../consumer/main_cpp.cpp )
elseif( UDA_PC_MODULE STREQUAL "uda-plugins" )
  set( _SOURCE main_plugin.cpp )
else()
  set( _SOURCE ../consumer/main.cpp )
endif()
add_executable( uda_pkgconfig_consumer ${_SOURCE} )
target_compile_features( uda_pkgconfig_consumer PRIVATE cxx_std_17 )
target_compile_options( uda_pkgconfig_consumer PRIVATE ${_PC_cflags} )
# Preserve pkg-config's archive order. It is significant for static consumers.
target_link_libraries( uda_pkgconfig_consumer PRIVATE ${_PC_LINK_LIBS} )

enable_testing()
add_test( NAME uda_pkgconfig_consumer COMMAND uda_pkgconfig_consumer )
