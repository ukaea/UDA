cmake_minimum_required( VERSION 2.8 )


link_directories(/Home/devarc/PortageMatlab7/tslib_client2013)

find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )

if( NOT MDSPLUS_FOUND )
  message( WARNING "mdsplus not found - skipping west plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping west plugin" )
  return()
endif()

include( plugins )

idam_plugin(
  NAME WEST
  ENTRY_FUNC westPlugin
  DESCRIPTION "Read WEST data"
  EXAMPLE "ADAS::adf21()"
  LIBNAME west_plugin
  SOURCES west_plugin.c west_xml.c ts_rqparm.c tsparse.c
  CONFIG_FILE west.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
    TS
)
