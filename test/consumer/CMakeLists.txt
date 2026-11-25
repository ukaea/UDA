cmake_minimum_required( VERSION 3.12 )
project( uda_installed_consumer LANGUAGES CXX )

set( UDA_CONSUMER_TARGET "UDA::client_static" CACHE STRING
  "Installed UDA target to exercise" )

set( _UDA_CONSUMER_MODULE_PATH "${CMAKE_MODULE_PATH}" )
if( UDA_CONSUMER_TARGET MATCHES "^UDA::cpp_" )
  find_package( UDACpp CONFIG REQUIRED )
  set( _SOURCE main_cpp.cpp )
else()
  find_package( UDA CONFIG REQUIRED COMPONENTS Client )
  set( _SOURCE main.cpp )
endif()
if( NOT "${CMAKE_MODULE_PATH}" STREQUAL "${_UDA_CONSUMER_MODULE_PATH}" )
  message( FATAL_ERROR "Finding UDA changed the consumer's CMAKE_MODULE_PATH" )
endif()

if( NOT TARGET ${UDA_CONSUMER_TARGET} )
  message( FATAL_ERROR
    "Requested consumer target '${UDA_CONSUMER_TARGET}' is not available" )
endif()

add_executable( uda_installed_consumer ${_SOURCE} )
target_link_libraries( uda_installed_consumer PRIVATE ${UDA_CONSUMER_TARGET} )

enable_testing()
add_test( NAME uda_installed_consumer COMMAND uda_installed_consumer )
