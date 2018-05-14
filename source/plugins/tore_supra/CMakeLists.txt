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
  NAME TORE
  ENTRY_FUNC tsPlugin
  DESCRIPTION "Read Tore Supra data"
  EXAMPLE "ADAS::adf21()"
  LIBNAME tore_supra_plugin
  SOURCES tore_supra_plugin.c ts_mds.c ts_xml.c
  CONFIG_FILE tore_supra.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
)
