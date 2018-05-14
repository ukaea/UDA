find_package( LibXml2 QUIET )
find_package( PPF QUIET )

if( NOT PPF_FOUND )
  message( WARNING "PPF not found - skipping ppf plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping ppf plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME PPF2
  ENTRY_FUNC plugin_entry
  DESCRIPTION "PPF Data"
  EXAMPLE "PPF2::read()"
  LIBNAME ppf_plugin
  SOURCES ppf_vars.c ppfplugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${LIBPPF_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${LIBPPF_LIBRARY}
)
