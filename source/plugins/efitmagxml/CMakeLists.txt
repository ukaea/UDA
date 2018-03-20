find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping efitmagxml plugin" )
  return()
endif()

include( plugins )

add_definitions( -Wno-error )

uda_plugin(
  NAME EFITMAGXML
  ENTRY_FUNC efitmagxml
  DESCRIPTION "Legacy EFIT++ Magnetics XML reader"
  EXAMPLE "EFITMAGXML::get()"
  LIBNAME efitmagxml_plugin
  CONFIG_FILE efitmagxml.cfg
  SOURCES
    initXMLStructs.c    
    allocXMLData.c
    efitmagxmllib.c
    parseHData.c
    getXPathValue.c
    efitmagxml.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
  EXTRA_INSTALL_FILES
    xml/idam.xml
    xml/jet.xml
    xml/mast.xml
)
