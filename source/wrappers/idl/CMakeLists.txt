cmake_minimum_required( VERSION 2.8 )

find_package( IDL REQUIRED )
find_package( PostgreSQL )
find_package( LibXml2 )
find_package( NetCDF )

set( LIB_NAME ${PROJECT_NAME}_idl )

set( SRC_FILES idam_dlm.c idamStructs.c )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${IDL_INCLUDES}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

if( PostgreSQL_FOUND )
  add_subdirectory( mastcpf )
else()
  message( WARNING "PostgreSQL not found - IDL mastcpf library not building" )
endif()

if( PostgreSQL_FOUND AND LibXml2_FOUND )
  add_subdirectory( roidam )
else()
  message( WARNING "PostgreSQL or LibXml2 not found - IDL roidam library not building" )
endif()

if( NetCDF_FOUND AND LibXml2_FOUND )
  add_subdirectory( putdata )
else()
  message( WARNING "NetCDF4 or LibXml2 not found - IDL netcdf4 library not building" )
endif()

target_link_libraries( ${LIB_NAME} LINK_PUBLIC client-static ${IDL_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  ${PROJECT_NAME}.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )

set( INSTALL_FILES
  getstruct.pro
  getdata.pro
  callplugin.pro
  geometry/addsignaldata.pro
  geometry/applygeommanipulation.pro
  geometry/cylindrical_cartesian.pro
  geometry/getgeomdata.pro
  geometry/getgeomsignaldata.pro
  geometry/pickupmanipulation.pro
  geometry/strnumber.pro
)

install( FILES ${INSTALL_FILES}
  DESTINATION idl )
