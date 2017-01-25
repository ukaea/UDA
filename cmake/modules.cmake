macro( find_modules )

  ########################################
  # Find available modules

  find_package( IDA )
  if( IDA_FOUND )
    include_directories( ${IDA_INCLUDE_DIR} )
  else()
    add_definitions( -DNOIDAPLUGIN )
  endif()

  find_package( PPF )
  if( PPF_FOUND )
    include_directories( ${LIBPPF_INCLUDE_DIR} )
  else()
    add_definitions( -DNOPPFPLUGIN )
    add_definitions( -DNOJPFPLUGIN )
  endif()

  find_package( HDF5 COMPONENTS C HL )
  if( HDF5_FOUND )
    include_directories( ${HDF5_INCLUDE_DIRS} )
  else()
    add_definitions( -DNOHDF5PLUGIN )
  endif()

  find_package( NetCDF )
  if( NETCDF_FOUND )
    include_directories( ${NETCDF_INCLUDES} )
  else()
    add_definitions( -DNONETCDFPLUGIN )
  endif()

  find_package( MDSplus )
  if( NOT MDSplus_FOUND )
    add_definitions( -DNOMDSPLUSPLUGIN )
  else( )
    include_directories( ${MDSPLUS_INCLUDES} )
  endif()

  find_package( PostgreSQL )
  if( NOT PostgreSQL_FOUND )
    add_definitions( -DNOTGENERICENABLED )
  else()
    include_directories( ${PostgreSQL_INCLUDE_DIRS} )
  endif()

endmacro( find_modules )

macro( link_modules TARGET_NAME )

  find_package( IDA )
  if( IDA_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${IDA_LIBRARIES} )
  endif()

  find_package( PPF )
  if( PPF_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${LIBPPF_LIBRARIES} )
  endif()

  find_package( HDF5 COMPONENTS C HL )
  if( HDF5_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${HDF5_C_LIBRARIES} ${HDF5_HL_LIBRARIES} )
  endif()

  find_package( NetCDF )
  if( NETCDF_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${NETCDF_LIBRARIES} )
  endif()

  find_package( MDSplus )
  if( MDSplus_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${MDSPLUS_LIBRARIES} )
  endif()

  find_package( PostgreSQL )
  if( PostgreSQL_FOUND )
    target_link_libraries( ${TARGET_NAME} LINK_PUBLIC ${PostgreSQL_LIBRARY} )
  endif()

endmacro( link_modules )
