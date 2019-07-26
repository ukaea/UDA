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
find_package( NetCDF QUIET )
find_package( UDUNITS QUIET )
if( MINGW )
  CONVERT_WIN_PATH( LIBXML2_INCLUDE_DIR )
  CONVERT_WIN_PATH( LIBXML2_LIBRARIES )
endif()

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping putdata plugin" )
  return()
elseif( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping putdata plugin" )
  return()
elseif( NOT UDUNITS_FOUND )
  message( WARNING "UDUNITS not found - skipping putdata plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME PUTDATA
  ENTRY_FUNC putdataPlugin
  DESCRIPTION "Save MAST data to NetCDF4 files"
  EXAMPLE "PUTDATA::open()"
  LIBNAME putdata_plugin
  SOURCES
    putdataPlugin.c
    putOpenClose.c
    putGroup.c
    putVariable.c
    putUnits.c
    putAttribute.c
    putDevice.c
    putCoordinate.c
  CONFIG_FILE
    putdata.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${NETCDF_INCLUDES}
    ${UDUNITS_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${NETCDF_LIBRARIES}
    ${UDUNITS_LIBRARIES}
)
