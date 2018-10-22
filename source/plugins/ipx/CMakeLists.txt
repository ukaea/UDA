find_package( Jasper QUIET )

if( NOT JASPER_FOUND )
  message( "Jasper not found - skipping IPX plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME NEWIPX
  ENTRY_FUNC ipxPlugin
  DESCRIPTION "IPX Data plugin"
  EXAMPLE ""
  LIBNAME ipx_plugin
  SOURCES ipx_plugin.cpp ipx.cpp jimage.c
  EXTRA_INCLUDE_DIRS
    ${JASPER_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${JASPER_LIBRARIES}
)