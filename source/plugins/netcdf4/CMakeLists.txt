########################################################################################################################
# Dependencies

find_package( LibXml2 QUIET )
find_package( NetCDF QUIET )
find_package( HDF5 COMPONENTS C HL QUIET )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping netcdf4 plugin" )
  return()
elseif( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping netcdf4 plugin" )
  return()
endif()

# HDF5 may be used by NetCDF
set( HDF5 )
if( HDF5_FOUND )
  set( HDF5 ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} )
endif()

# Use libcurl under Windows
set( CURL )
if( WIN32 OR MINGW )
  set( CURL curl ws2_32 )
endif()

include( plugins )

if( NO_MODULES )

uda_plugin(
  NAME NETCDF
  ENTRY_FUNC idamCDF4
  DESCRIPTION "netCDF4 Data Reader"
  EXAMPLE "NETCDF::get()"
  LIBNAME netcdf4_plugin
  SOURCES
    netCDF4.c netCDF4.h
    readCDF4.c readCDF4.h
    readCDF4SubTree.c readCDF4SubTree.h
    readCDFAtts.c readCDFAtts.h
    readCDFMeta.c readCDFMeta.h
    readCDFTypes.c
    readCDFVar.c
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${NETCDF_LIBRARIES}
	${HDF5}
	${CURL}
)

else()

uda_plugin(
  NAME NEWCDF4
  ENTRY_FUNC idamCDF4
  DESCRIPTION "netCDF4 Data Reader"
  EXAMPLE "NEWCDF4::get()"
  LIBNAME netcdf4_plugin
  SOURCES
    netCDF4.c netCDF4.h
  EXTRA_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/source/server/modules/netcdf4
    ${LIBXML2_INCLUDE_DIR}
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${NETCDF_LIBRARIES}
	${HDF5}
	${CURL}
)

endif()
