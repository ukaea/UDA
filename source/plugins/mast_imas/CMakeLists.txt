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
find_package( LibXml2 QUIET )
find_package( PostgreSQL QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping mast_imas plugin" )
  return()
elseif( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping mast_imas plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME MAST_IMAS
  ENTRY_FUNC mastImasPlugin
  DESCRIPTION "plugin to handle IMAS requests by returning MAST data"
  EXAMPLE "MAST_IMAS::read(element='magnetics/method/#/name', index=1)"
  LIBNAME mast_imas_plugin
  SOURCES mastImasPlugin.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    client-static
)
