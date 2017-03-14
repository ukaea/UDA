cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping IDAM plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME UDA
  ENTRY_FUNC UDAPlugin
  DESCRIPTION "forward on requests to the UDA client"
  EXAMPLE "UDA::get(host=idam0, port=56565, signal=ip, source=12345)"
  LIBNAME uda_plugin
  SOURCES uda_plugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    client-static
    ${SECURITY_LIBRARIES}
)
