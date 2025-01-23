find_package( HDF5 COMPONENTS C HL QUIET )
find_package( fmt REQUIRED )

if( NOT HDF5_FOUND )
  message( WARNING "HDF5 not found - skipping hdf5 plugin" )
  return()
endif()

include( udaPlugins )

uda_plugin(
  NAME HDF5
  VERSION 1.0.0
  ENTRY_FUNC hdf5Plugin
  DESCRIPTION "HDF5 Data Reader"
  EXAMPLE "HDF5::read()"
  LIBNAME hdf5_plugin
  SOURCES hdf5plugin.cpp hdf5plugin.h readHDF58.cpp readHDF58.h
  EXTRA_LINK_LIBS hdf5::hdf5 hdf5::hdf5_hl fmt::fmt
)