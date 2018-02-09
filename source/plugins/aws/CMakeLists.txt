cmake_minimum_required( VERSION 2.8 )

#find_package( AWS3 QUIET )
find_package( AWS3 )

if( NOT LIBAWS3_FOUND )
  message( WARNING "AWS installation not found - skipping AWS plugin" )
  return()
endif()

include( plugins )

add_definitions( -Wno-error -std=c++11)

uda_plugin(
  NAME AWS
  ENTRY_FUNC services
  DESCRIPTION "Amazon AWS S3 services"
  EXAMPLE "s3::get()"
  LIBNAME aws_plugin
  CONFIG_FILE aws3.cfg
  SOURCES
    aws3.cpp aws3.h
  EXTRA_INCLUDE_DIRS
    ${LIBAWS3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBAWS3_LIBRARIES}
    curl ssl uuid z
  EXTRA_INSTALL_FILES
)
