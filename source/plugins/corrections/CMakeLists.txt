# Macro used to convert Windows path to Unix
macro( CONVERT_WIN_PATH VarName )
  if( MINGW AND NOT "${${VarName}}" STREQUAL "" )
    execute_process( COMMAND cygpath.exe -u ${${VarName}} OUTPUT_VARIABLE ${VarName} )
    string( STRIP ${${VarName}} ${VarName} )
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
  message( WARNING "Libxml2 not found - skipping corrections plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME ANBCORRECTIONS
  ENTRY_FUNC anbCorrections
  DESCRIPTION "Corrections to ANB Data"
  EXAMPLE "ANBCORRECTIONS::chequerBoardPower(signal=ANB_SW_FULL_POWER)"
  LIBNAME corrections_plugin
  SOURCES anbCorrections.c
  CONFIG_FILE anbCorrections.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    client-static
    m
)
