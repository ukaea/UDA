link_directories(/Home/devarc/PortageMatlab7/tslib_client2013)

find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )
find_package( LibTS QUIET )

if( NOT MDSPLUS_FOUND )
  message( WARNING "mdsplus not found - skipping west plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping west plugin" )
  return()
elseif( NOT LIBTS_FOUND )
  message( WARNING "LibTS not found - skipping west plugin" )
  return()
endif()

add_definitions( -Wno-error )

include( plugins )

uda_plugin(
  NAME WEST
  ENTRY_FUNC westPlugin
  DESCRIPTION "Read WEST data"
  EXAMPLE "ADAS::adf21()"
  LIBNAME west_plugin
  SOURCES west_plugin.c west_xml.c ts_rqparm.c tsparse.c west_ece.c west_dynamic_data.c west_utilities.c west_ece_mode.c west_dyn_data_utilities.c west_pf_active.c west_pf_passive.c west_static_data_utilities.c west_soft_x_rays.c west_summary.c west_lh_antennas.c west_barometry.c 
  CONFIG_FILE west.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${MDSPLUS_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${MDSPLUS_LIBRARIES}
    ${LIBTS_LIBRARIES}
)
