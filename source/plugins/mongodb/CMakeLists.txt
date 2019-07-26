# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
    string( REGEX REPLACE "[\r\n]+" ";" ${VarName} ${${VarName}} )
  endif()
endmacro()

########################################################################################################################
# Dependencies
find_package( MongoC QUIET )
find_package( LibXml2 QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT LIBMONGOC_FOUND )
  message( WARNING "MongoC not found - skipping MongoDB plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping MongoDB plugin" )
  return()
endif()

include( plugins )

get_filename_component( LIBMONGOC_LIBRARY_DIR ${LIBMONGOC_LIBRARY} DIRECTORY )

uda_plugin(
  NAME MONGO
  ENTRY_FUNC query
  DESCRIPTION "Query the MongoDB database"
  EXAMPLE "MONGO::query()"
  LIBNAME mongodb_plugin
  SOURCES mongodbplugin.c
  CONFIG_FILE mongodb.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${LIBMONGOC_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${LIBMONGOC_LIBRARIES}
)
