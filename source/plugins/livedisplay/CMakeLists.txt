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
  message( WARNING "Libxml2 not found - skipping LiveDisplay plugin" )
  return()
endif()

include( plugins )

add_definitions( -Wno-error )

uda_plugin(
  NAME LIVEDISPLAY
  ENTRY_FUNC livedisplay
  DESCRIPTION "Live Display IMAS MAGNETICS IDS plugin"
  EXAMPLE "LIVEDISPLAY::get()"
  LIBNAME livedisplay_plugin
  SOURCES livedisplay.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/clientserver
    ${CMAKE_SOURCE_DIR}/structures
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    client-static
)
