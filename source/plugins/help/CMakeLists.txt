########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( fmt REQUIRED )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping help plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET )
endif()

include( plugins )

uda_plugin(
  NAME HELP
  ENTRY_FUNC helpPlugin
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES help_plugin.cpp
  EXTRA_LINK_LIBS
    LibXml2::LibXml2
    fmt::fmt
)
