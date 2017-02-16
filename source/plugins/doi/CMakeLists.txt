cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping doi plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping doi plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME DOI
  ENTRY_FUNC issueDOI
  DESCRIPTION "Digital Object Identifiers"
  EXAMPLE "DOI::issue()"
  LIBNAME doi_plugin
  SOURCES issueDOI.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
