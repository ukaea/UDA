find_package( NetCDF QUIET )
find_package( fmt QUIET )

if( NOT NetCDF_FOUND )
  message( WARNING "NetCDF not found - skipping netcdf plugin" )
  return()
elseif( NOT fmt_FOUND )
  message( WARNING "fmt not found - skipping netcdf4 plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME VALUE
  ENTRY_FUNC netcdfPlugin
  DESCRIPTION "Plugin for saving and loading from a netcdf store"
  EXAMPLE "NETCDF::save(key='keyName', value='valueData')"
  LIBNAME netcdf_plugin
  SOURCES netcdf_plugin.cpp read_netcdf.cpp write_uda.cpp
  EXTRA_INCLUDE_DIRS
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${NETCDF_LIBRARIES}
    fmt::fmt
)
