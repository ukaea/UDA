find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

uda_plugin(
  NAME TEST_PLUGIN
  VERSION 1.0.0
  ENTRY_FUNC testPlugin
  DESCRIPTION "Generate Test Data"
  EXAMPLE "TEST_PLUGIN::test1()"
  LIBNAME testplugin_plugin
  SOURCES testplugin.cpp teststructs.cpp
  EXTRA_LINK_LIBS Boost::filesystem fmt::fmt
)