find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

uda_plugin(
  NAME HELP
  VERSION 1.0.0
  ENTRY_FUNC helpPlugin
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES help_plugin.cpp
  EXTRA_LINK_LIBS Boost::filesystem fmt::fmt
)