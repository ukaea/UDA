cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping viewport plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping viewport plugin" )
  return()
endif()

include( plugins )

idam_plugin(
  NAME VIEWPORT
  ENTRY_FUNC viewport
  DESCRIPTION "Reduce data to viewport pixel size"
  EXAMPLE "viewport::get()"
  LIBNAME viewport_plugin
  SOURCES viewport.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
