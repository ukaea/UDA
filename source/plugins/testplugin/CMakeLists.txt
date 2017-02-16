cmake_minimum_required( VERSION 2.8 )

find_package( LibXml2 QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping test plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME TESTPLUGIN
  ENTRY_FUNC testplugin
  DESCRIPTION "Generate Test Data"
  EXAMPLE "TESTPLUGIN::test1()"
  LIBNAME test_plugin
  SOURCES testplugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
