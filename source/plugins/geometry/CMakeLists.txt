cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )
find_package( NetCDF QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping Geometry plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping Geometry plugin" )
  return()
elseif( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping Geometry plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME GEOM
  ENTRY_FUNC idamGeom
  DESCRIPTION "Reads in geometry db netcdf files"
  EXAMPLE "GEOM::get()"
  LIBNAME geometry_plugin
  SOURCES 
    geometry.c
    geomSignalMap.c
    geomConfig.c
  CONFIG_FILE geometry.cfg
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${NETCDF_LIBRARIES}
    m
)
