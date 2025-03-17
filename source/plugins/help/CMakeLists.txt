find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

configure_file( help_plugin_interface.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/help_plugin_interface.cpp @ONLY )

uda_plugin(
  NAME HELP
  VERSION 1.0.0
  ENTRY_FUNC helpPlugin
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES help_plugin.cpp ${CMAKE_CURRENT_BINARY_DIR}/help_plugin_interface.cpp
  EXTRA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  EXTRA_LINK_LIBS Boost::filesystem fmt::fmt
)
