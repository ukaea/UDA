cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL QUIET )
find_package( LibXml2 QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )

if( NOT PostgreSQL_FOUND )
  message( WARNING "PostgreSQL not found - skipping hdf5 plugin" )
  return()
elseif( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping hdf5 plugin" )
  return()
elseif( NOT HDF5_FOUND )
  message( WARNING "HDF5 not found - skipping hdf5 plugin" )
  return()
endif()

include( plugins )

#message( FATAL_ERROR ">>>> ${HDF5_LIBRARIES}" )

idam_plugin(
  NAME NEWHDF5
  ENTRY_FUNC idamHDF5
  DESCRIPTION "HDF5 Data Reader"
  EXAMPLE "NEWHDF5::get()"
  LIBNAME hdf5_plugin
  SOURCES hdf5plugin.c readHDF58.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${HDF5_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    ${HDF5_LIBRARIES}
    ${HDF5_HL_LIBRARIES}
)
