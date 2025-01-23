find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

uda_plugin(
  NAME TEMPLATE_PLUGIN
  VERSION 1.0.0
  ENTRY_FUNC templatePlugin
  DESCRIPTION "Standardised Plugin Template"
  EXAMPLE "TEMPLATE_PLUGIN::functionName()"
  LIBNAME template_plugin
  SOURCES templatePlugin.cpp
  EXTRA_LINK_LIBS Boost::filesystem fmt::fmt
)