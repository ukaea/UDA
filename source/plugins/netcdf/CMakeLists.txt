find_package( NetCDF QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )
find_package( fmt QUIET )

if( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping netcdf4 plugin" )
  return()
elseif( NOT fmt_FOUND )
  message( WARNING "fmt not found - skipping netcdf4 plugin" )
  return()
endif()

# HDF5 may be used by NetCDF
set( HDF5 )
if( HDF5_FOUND )
  set( HDF5 ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} )
endif()

include( plugins )

uda_plugin(
  NAME NEW_NETCDF
  EXTRA_NAMES CDF7 NCDF7 HDF57
  ENTRY_FUNC capnp_netcdf
  DESCRIPTION "netCDF4 Data Reader"
  EXAMPLE "NEW_NETCDF::read()"
  LIBNAME new_netcdf_plugin
  SOURCES
    netcdf_plugin.cpp netcdf_plugin.hpp
    netcdf_reader.cpp netcdf_reader.hpp
    capnp_reader.cpp capnp_reader.hpp
    netcdfpp.h
  EXTRA_INCLUDE_DIRS
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${NETCDF_LIBRARIES}
	  ${HDF5}
    fmt::fmt
)