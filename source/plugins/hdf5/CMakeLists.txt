########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping hdf5 plugin" )
  return()
elseif( NOT HDF5_FOUND )
  message( WARNING "HDF5 not found - skipping hdf5 plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME NEWHDF5
  ENTRY_FUNC udaHDF5
  DESCRIPTION "HDF5 Data Reader"
  EXAMPLE "NEWHDF5::read()"
  LIBNAME hdf5_plugin
  SOURCES hdf5plugin.cpp hdf5plugin.h readHDF58.cpp readHDF58.h
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${HDF5_INCLUDE_DIRS}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${HDF5_LIBRARIES}
    ${HDF5_HL_LIBRARIES}
)
