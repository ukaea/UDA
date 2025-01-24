find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

uda_plugin(
  NAME UDA
  VERSION 1.0.0
  ENTRY_FUNC UDAPlugin
  DESCRIPTION "forward on requests to the UDA client"
  EXAMPLE "UDA::get(host=idam0, port=56565, signal=ip, source=12345)"
  LIBNAME uda_plugin
  SOURCES uda_plugin.cpp
  EXTRA_LINK_LIBS uda::client Boost::filesystem fmt::fmt
)