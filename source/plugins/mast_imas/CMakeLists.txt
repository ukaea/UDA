cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )
find_package( PostgreSQL QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping mast_imas plugin" )
  return()
elseif( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping mast_imas plugin" )
  return()
endif()

include( plugins )

idam_plugin(
  NAME MAST_IMAS
  ENTRY_FUNC mastImasPlugin
  DESCRIPTION "plugin to handle IMAS requests by returning MAST data"
  EXAMPLE "MAST_IMAS::read(element='magnetics/method/#/name', index=1)"
  LIBNAME mast_imas_plugin
  SOURCES mastImasPlugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
