########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( LevelDB QUIET )

if( NOT LevelDB_FOUND )
  message( WARNING "LevelDB not found - skipping keyvalue plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping template plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME VALUE
  ENTRY_FUNC keyValue
  DESCRIPTION "Plugin for saving and loading from a key value store"
  EXAMPLE "KEYVALUE::save(key='keyName', value='valueData')"
  LIBNAME keyvalue_plugin
  SOURCES keyvaluePlugin.cpp
  CONFIG_FILE keyvalue.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${LevelDB_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${LevelDB_LIBRARIES}
)
