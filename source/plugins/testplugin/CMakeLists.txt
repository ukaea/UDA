########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( fmt REQUIRED )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping test plugin" )
  return()
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  include_directories( ${XDR_INCLUDE_DIR} )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
    include_directories( ${TIRPC_INCLUDE_DIR} )
  endif()
endif()

include( plugins )

uda_plugin(
  NAME TESTPLUGIN
  ENTRY_FUNC testPlugin
  DESCRIPTION "Generate Test Data"
  EXAMPLE "TESTPLUGIN::test1()"
  LIBNAME testplugin_plugin
  SOURCES testplugin.cpp teststructs.cpp
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    fmt::fmt
)
