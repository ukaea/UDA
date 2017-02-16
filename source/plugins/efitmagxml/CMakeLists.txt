cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping efitmagxml plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME EFITMAGXML
  ENTRY_FUNC efitmagxml
  DESCRIPTION "Legacy EFIT++ Magnetics XML reader"
  EXAMPLE "EFITMAGXML::get()"
  LIBNAME efitmagxml_plugin
  SOURCES
    initXMLStructs.c    
    allocXMLData.c
    efitmagxmllib.c
    parseHData.c
    efitmagxml.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
