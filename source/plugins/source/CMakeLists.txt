include( plugins )

add_definitions( -Wno-error )

uda_plugin(
  NAME SOURCE
  ENTRY_FUNC source_plugin
  DESCRIPTION "Local data dource argument Generator for UDA Data requests"
  EXAMPLE "SOURCE::get()"
  LIBNAME source_plugin
  CONFIG_FILE source.cfg
  SOURCES
    source.c
  EXTRA_INCLUDE_DIRS
  EXTRA_LINK_LIBS
  EXTRA_INSTALL_FILES
)
