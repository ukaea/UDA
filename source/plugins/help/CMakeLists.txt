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
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping help plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME HELP
  ENTRY_FUNC idamServerHelp
  DESCRIPTION "Service Discovery: list the details on all registered services"
  EXAMPLE "HELP::services()"
  LIBNAME help_plugin
  SOURCES idamServerHelp.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
)
