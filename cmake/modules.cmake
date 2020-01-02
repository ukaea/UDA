macro( find_modules )

  if( NOT NO_MODULES )

    ########################################
    # Find available modules

    find_package( IDA )
    if( IDA_FOUND )
      include_directories( SYSTEM ${IDA_INCLUDE_DIR} )
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

  else()

    add_definitions( -DNOIDAPLUGIN )
    add_definitions( -DNOPPFPLUGIN )
    add_definitions( -DNOJPFPLUGIN )
    add_definitions( -DNOHDF5PLUGIN )
    add_definitions( -DNONETCDFPLUGIN )
    add_definitions( -DNOMDSPLUSPLUGIN )
    add_definitions( -DNOTGENERICENABLED )

  endif()

endmacro( find_modules )

macro( link_modules TARGET_NAME )

  if( NOT NO_MODULES )

    find_package( IDA QUIET )
    if( IDA_FOUND )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${IDA_LIBRARIES} )
    endif()

    find_package( PPF QUIET )
    if( PPF_FOUND )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${LIBPPF_LIBRARIES} )
    endif()

    find_package( HDF5 COMPONENTS C HL QUIET )
    if( HDF5_FOUND )
      link_directories( ${HDF5_LIBRARY_DIRS} )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${HDF5_C_LIBRARIES} ${HDF5_HL_LIBRARIES} )
    endif()

    find_package( NetCDF QUIET )
    if( NETCDF_FOUND )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${NETCDF_LIBRARIES} )
    endif()

    find_package( MDSplus QUIET )
    if( MDSplus_FOUND )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${MDSPLUS_LIBRARIES} )
    endif()

    find_package( PostgreSQL QUIET )
    if( PostgreSQL_FOUND )
      link_directories( ${PostgreSQL_LIBRARY_DIRS} )
      target_link_libraries( ${TARGET_NAME} PUBLIC ${PostgreSQL_LIBRARY} )
    endif()

  endif()

endmacro( link_modules )
