########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping template plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME TEMPLATEPLUGIN
  ENTRY_FUNC templatePlugin
  DESCRIPTION "Standardised Plugin Template"
  EXAMPLE "TEMPLATEPLUGIN::functionName()"
  LIBNAME template_plugin
  SOURCES templatePlugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
