########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( fmt REQUIRED )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping IDAM plugin" )
  return()
endif()

set( LIBS LibXml2::LibXml2 fmt::fmt )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

include( plugins )

uda_plugin(
  NAME UDA
  ENTRY_FUNC UDAPlugin
  DESCRIPTION "forward on requests to the UDA client"
  EXAMPLE "UDA::get(host=idam0, port=56565, signal=ip, source=12345)"
  LIBNAME uda_plugin
  SOURCES uda_plugin.cpp
  EXTRA_LINK_LIBS
    ${LIBS}
)
