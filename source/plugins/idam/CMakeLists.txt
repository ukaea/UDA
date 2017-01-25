cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping IDAM plugin" )
  return()
endif()

set( LIB_NAME idam_plugin )

include( plugins )

idam_plugin(
  NAME XDAM
  ENTRY_FUNC idamAPIPlugin
  DESCRIPTION "use the idamAPIPlugin plugin with the idamAPI method"
  EXAMPLE "XDAM::get(host=idam0, port=56565, signal=ip, source=12345)"
  LIBNAME ${LIB_NAME}
  SOURCES idamAPIPlugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

if( NOT FAT_BUILD )
  target_link_libraries( ${LIB_NAME} LINK_PUBLIC server )
else()
  target_link_libraries( ${LIB_NAME}
    ${CMAKE_INSTALL_PREFIX}/lib/lib${PROJECT_NAME}.so
    ${CMAKE_INSTALL_PREFIX}/lib/lib${PROJECT_NAME}_server.so
    )
endif()
