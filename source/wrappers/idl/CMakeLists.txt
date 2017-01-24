cmake_minimum_required( VERSION 2.8 )

find_package( IDL REQUIRED )
find_package( PostgreSQL )
find_package( LibXml2 )
find_package( NetCDF )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME idamidl )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SRC_FILES idam.c idamStructs.c )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${SRC_DIR}/client
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

link_directories( ${LIB_DIR} )
target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} ${IDL_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME idam
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  idam.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )

set( INSTALL_FILES
  getstruct.pro
  getdata.pro
  addsignaldata.pro
  applygeommanipulation.pro
  cylindrical_cartesian.pro
  getgeomdata.pro
  getgeomsignaldata.pro
  pickupmanipulation.pro
  strnumber.pro
)

install( FILES ${INSTALL_FILES}
  DESTINATION idl )
