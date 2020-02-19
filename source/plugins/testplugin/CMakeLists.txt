########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping test plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  include_directories( ${XDR_INCLUDE_DIR} )
endif()

include( plugins )

uda_plugin(
  NAME TESTPLUGIN
  ENTRY_FUNC testplugin
  DESCRIPTION "Generate Test Data"
  EXAMPLE "TESTPLUGIN::test1()"
  LIBNAME testplugin_plugin
  SOURCES testplugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
