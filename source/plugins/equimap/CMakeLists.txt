cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping equimap plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping equimap plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME EQUIMAP
  ENTRY_FUNC equiMap
  DESCRIPTION "Map Profile Data"
  EXAMPLE "EQUIMAP::ayc_te()"
  LIBNAME equimap_plugin
  SOURCES equimap.c xdata.c importdata.c smoothpsi.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/wrappers/idl/scruncher/newscruncher
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    m
    ${PostgreSQL_LIBRARY}
    client-static
    ${SECURITY_LIBRARIES}
)
