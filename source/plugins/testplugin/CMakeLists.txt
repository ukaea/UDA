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
else()
  find_package( TIRPC QUIET )
endif()

include( plugins )

uda_plugin(
  NAME TESTPLUGIN
  ENTRY_FUNC testplugin
  DESCRIPTION "Generate Test Data"
  EXAMPLE "TESTPLUGIN::test1()"
  LIBNAME testplugin_plugin
  SOURCES testplugin.cpp teststructs.cpp
  EXTRA_LINK_LIBS
    LibXml2::LibXml2
    fmt::fmt
)
