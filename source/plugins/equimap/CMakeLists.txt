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
  message( WARNING "PostgreSQL not found - skipping equimap plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping equimap plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME EQUIMAP
  ENTRY_FUNC equiMap
  DESCRIPTION "Map Profile Data"
  EXAMPLE "EQUIMAP::ayc_te()"
  LIBNAME equimap_plugin
  SOURCES equimap.c xdata.c importdata.c smoothpsi.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/wrappers/idl/scruncher/newscruncher
    ${CMAKE_SOURCE_DIR}/client
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    m
    ${PostgreSQL_LIBRARY}
    client-static
)