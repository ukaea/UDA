########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( MDSplus QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping MDSplus plugin" )
  return()
elseif( NOT MDSPLUS_FOUND )
  message( WARNING "MDSplus not found - skipping MDSplus plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME MDSPLUS
  ENTRY_FUNC mdsplusPlugin
  DESCRIPTION "MDSplus reader"
  EXAMPLE "MDSPLUS::read()"
  LIBNAME mdsplus_plugin
  SOURCES mdsplusPlugin.cpp
  EXTRA_LINK_LIBS
    LibXml2::LibXml2
    MDSPLUS::MDSPLUS
)
