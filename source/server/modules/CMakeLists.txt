########################################################################################################################
# Dependencies

include( modules )

find_modules()

########################################################################################################################
# Sources

set( SRC_FILES
  ida/readIda.c
  ida/nameIda.c
  ida/readIdaItem.c
  ida/parseIdaPath.c
  bytes/readBytesNonOptimally.c
  readNothing/readNothing.c
  hdf58/readHDF58.c
  jpf/readjpf.c
  ufile/readUFile.c
  idam/readIdam.c
  netcdf4/readCDF4.c
  readsql/readSQL.c
  ppf/readppf.c
  hdata/readHData.c
  mdsplus/readMDS.c
  )

if( MDSplus_FOUND )
  list( APPEND SRC_FILES mdsplus/readMDSDim.c )
endif()

if( NETCDF_FOUND )
  list( APPEND SRC_FILES
    netcdf4/readCDF4SubTree.c
    netcdf4/readCDFAtts.c
    netcdf4/readCDFMeta.c
    netcdf4/readCDFTypes.c
    netcdf4/readCDFVar.c
    )
endif()

include_directories(
  ${CMAKE_SOURCE_DIR}/source
)

########################################################################################################################
# Targets

add_library( modules-objects OBJECT ${SRC_FILES} )
target_compile_definitions( modules-objects PRIVATE -DSERVERBUILD )

add_library( fatmodules-objects OBJECT ${SRC_FILES} )
target_compile_definitions( fatmodules-objects PRIVATE -DFATCLIENT )
