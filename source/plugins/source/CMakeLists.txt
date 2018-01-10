cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping source plugin" )
  return()
endif()

include( plugins )

add_definitions( -Wno-error )

uda_plugin(
  NAME SOURCE
  ENTRY_FUNC source_plugin
  DESCRIPTION "Local data dource argument Generator for UDA Data requests"
  EXAMPLE "SOURCE::get()"
  LIBNAME source_plugin
  CONFIG_FILE source.cfg
  SOURCES
    source.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    client-static
  EXTRA_INSTALL_FILES
)
