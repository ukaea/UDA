########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( fmt REQUIRED )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping IDAM plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET )
endif()

include( plugins )

uda_plugin(
  NAME UDA
  ENTRY_FUNC UDAPlugin
  DESCRIPTION "forward on requests to the UDA client"
  EXAMPLE "UDA::get(host=idam0, port=56565, signal=ip, source=12345)"
  LIBNAME uda_plugin
  SOURCES uda_plugin.cpp
  EXTRA_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    LibXml2::LibXml2
    OpenSSL::SSL
    OpenSSL::Crypto
    fmt::fmt
)
