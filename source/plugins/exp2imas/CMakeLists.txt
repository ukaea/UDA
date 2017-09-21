cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )

if( NOT MDSPLUS_FOUND )
  message( WARNING "mdsplus not found - skipping tore_supra plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping tore_supra plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME EXP2IMAS
  ENTRY_FUNC exp2imasPlugin
  DESCRIPTION "Map IMAS element name to experiment static or dynamic data using XML mapping files"
  EXAMPLE "EXP2IMAS::read()"
  LIBNAME exp2imas_plugin
  SOURCES exp2imas_plugin.c exp2imas_mds.c exp2imas_xml.c
  CONFIG_FILE exp2imas.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
)