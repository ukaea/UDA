########################################################################################################################
# Dependencies

find_package( LevelDB QUIET )

if( NOT LevelDB_FOUND )
  message( WARNING "LevelDB not found - skipping keyvalue plugin" )
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
    ${LevelDB_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LevelDB_LIBRARIES}
)
