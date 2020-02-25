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

if( WIN32 OR MINGW )
  find_package( CURL REQUIRED )
  find_package( XDR REQUIRED )
  include_directories( ${XDR_INCLUDE_DIR} )
endif()

# HDF5 may be used by NetCDF
set( HDF5 )
if( HDF5_FOUND )
  set( HDF5 ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} )
endif()

# Use libcurl under Windows
set( CURL )
if( WIN32 OR MINGW )
  set( CURL ${CURL_LIBRARIES} ws2_32 )
endif()

# Link with Server Objects
set( SRV_OBJ )
if( WIN32 OR MINGW )
  set( SRV_OBJ server-objects logging-server-objects )
else()
  set( SRV_OBJ server-static )
endif()

include( plugins )

uda_plugin(
  NAME NETCDF
  EXTRA_NAMES CDF NCDF
  ENTRY_FUNC netCDF4
  DESCRIPTION "netCDF4 Data Reader"
  EXAMPLE "NETCDF::get()"
  LIBNAME netcdf4_plugin
  SOURCES
    netCDF4.cpp netCDF4.hpp
    readCDF4.cpp readCDF4.hpp
    readCDF4SubTree.cpp readCDF4SubTree.hpp
    readCDFAtts.cpp readCDFAtts.hpp
    readCDFMeta.cpp readCDFMeta.hpp
    readCDFTypes.cpp
    readCDFVar.cpp
  EXTRA_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
    ${NETCDF_INCLUDES}
  EXTRA_LINK_LIBS
    ${LIBXML2_LIBRARIES}
    ${NETCDF_LIBRARIES}
    ${HDF5}
    ${CURL}
    ${SRV_OBJ}
)
