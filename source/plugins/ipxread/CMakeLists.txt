cmake_minimum_required( VERSION 2.8 )

include( plugins )

uda_plugin(
  NAME IPXREAD
  ENTRY_FUNC udaIpx
  DESCRIPTION "data reader to access ipx files"
  EXAMPLE "IPXREAD::read()"
  LIBNAME ipx_plugin
  SOURCES ipxPlugin.c
  CONFIG_FILE ipxread.cfg
)
