cmake_minimum_required( VERSION 2.8 )

find_package( LIBS3 QUIET )

if( NOT LIBS3_FOUND )
  message( WARNING "LIBS3 installation not found - skipping LIBS3 plugin" )
  return()
endif()

#message( "LIBS3 installation found" )
message( ${LIBS3_LIBRARY} )

include( plugins )

add_definitions( -Wno-error)

uda_plugin(
  NAME LIBS3
  ENTRY_FUNC s3
  DESCRIPTION "LIBS3 S3 API"
  EXAMPLE "s3::get()"
  LIBNAME s3_plugin
  CONFIG_FILE s3.cfg
  SOURCES
    s3.c s3.h
  EXTRA_INCLUDE_DIRS
    ${LIBS3_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBS3_LIBRARY}
    /home/localadmin/S3/lib/libs3.so
  EXTRA_INSTALL_FILES
)
