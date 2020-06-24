########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping viewport plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  pkg_check_modules(XDR "libtirpc")
endif()
  include_directories( ${XDR_INCLUDE_DIR} )


include( plugins )

uda_plugin(
  NAME VIEWPORT
  ENTRY_FUNC viewport
  DESCRIPTION "Reduce data to viewport pixel size"
  EXAMPLE "viewport::get()"
  LIBNAME viewport_plugin
  SOURCES viewport.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)