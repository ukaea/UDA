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
find_package( IDA QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT IDA_FOUND )
  message( WARNING "IDA not found - skipping IDA3 plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping IDA3 plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME NEWIDA
  ENTRY_FUNC ida3plugin
  DESCRIPTION "IDA Data plugin"
  EXAMPLE ""
  LIBNAME ida3_plugin
  SOURCES ida3plugin.c readIda.c nameIda.c readIdaItem.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    SYSTEM ${IDA_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${IDA_LIBRARIES}
)
