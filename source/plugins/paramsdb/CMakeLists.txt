########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( PostgreSQL QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping ParamsDB plugin" )
  return()
elseif( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping ParamsDB plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME PARAMSDB
  ENTRY_FUNC paramsdb
  DESCRIPTION "Plugin for accessing MAST real time controls parameters database"
  EXAMPLE "PARAMSDB::help()"
  LIBNAME paramsdb_plugin
  SOURCES paramsdb.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
