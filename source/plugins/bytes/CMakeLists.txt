cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping template plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME BYTES
  ENTRY_FUNC bytesPlugin
  DESCRIPTION "data reader to access files as a block of bytes without interpretation"
  EXAMPLE "BYTES::read()"
  LIBNAME bytes_plugin
  SOURCES bytesPlugin.c
  EXTRA_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/source/server/modules/bytes
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
