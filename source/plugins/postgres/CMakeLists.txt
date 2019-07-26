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
find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping postgres plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping postgres plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME POSTGRES
  ENTRY_FUNC postgres_query
  DESCRIPTION "Queries a metadata catalogue for name mappings"
  EXAMPLE "POSTGRES::query(signal=ip)"
  LIBNAME postgres_plugin
  SOURCES postgresplugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
)
