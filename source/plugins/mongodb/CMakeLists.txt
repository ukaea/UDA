cmake_minimum_required( VERSION 2.8 )

find_package( MongoC QUIET )
find_package( LibXml2 QUIET )

if( NOT LIBMONGOC_FOUND )
  message( WARNING "MongoC not found - skipping MongoDB plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping MongoDB plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME MONGO
  ENTRY_FUNC query
  DESCRIPTION "Query the MongoDB database"
  EXAMPLE "MONGO::query()"
  LIBNAME mongodb_plugin
  SOURCES mongodbplugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${LIBMONGOC_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${LIBMONGOC_LIBRARIES}
)
