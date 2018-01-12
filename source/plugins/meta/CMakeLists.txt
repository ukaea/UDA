cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping meta plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping meta plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME META
  ENTRY_FUNC readMeta
  DESCRIPTION "Meta Data Plugin"
  EXAMPLE "META::listMeta()"
  LIBNAME meta_plugin
  SOURCES readMeta.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    client-static
)
