########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping help plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  include_directories( ${XDR_INCLUDE_DIR} )
endif()

include( plugins )

uda_plugin(
  NAME HELP
  ENTRY_FUNC idamServerHelp
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES idamServerHelp.cpp
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
