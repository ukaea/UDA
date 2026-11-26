cmake_minimum_required( VERSION 3.12 )
project( uda_installed_consumer LANGUAGES CXX )

# Each installed target gets its own executable, so one configure and one build check the whole package.
find_package( UDA CONFIG REQUIRED COMPONENTS Client )
find_package( UDACpp CONFIG REQUIRED )

enable_testing()

foreach( _TARGET IN ITEMS UDA::client_static UDA::client_shared UDA::cpp_static UDA::cpp_shared )
  if( NOT TARGET ${_TARGET} )
    message( FATAL_ERROR "Installed target '${_TARGET}' is not available" )
  endif()

  string( REPLACE "::" "_" _NAME "${_TARGET}" )
  if( _TARGET MATCHES "cpp" )
    set( _SOURCE main_cpp.cpp )
  else()
    set( _SOURCE main.cpp )
  endif()

  add_executable( ${_NAME} ${_SOURCE} )
  target_link_libraries( ${_NAME} PRIVATE ${_TARGET} )
  add_test( NAME ${_NAME} COMMAND ${_NAME} )
endforeach()
