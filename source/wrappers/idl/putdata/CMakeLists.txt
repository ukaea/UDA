cmake_minimum_required( VERSION 2.8 )

find_package( IDL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( NetCDF QUIET )
find_package( UDUNITS QUIET )

if( NOT NETCDF_FOUND )
  message( WARNING "NetCDF not found - skipping putdata IDL DLM library" )
  return()
elseif( NOT UDUNITS_FOUND )
  message( WARNING "UDUNITS not found - skipping putdata IDL DLM library" )
  return()
endif()

set( LIB_NAME putdata )

set( SRC_FILES
  putdata.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${LIBXML2_INCLUDE_DIR}
  ${IDL_INCLUDES}
  ${NETCDF_INCLUDES}
  ${UDUNITS_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

target_link_libraries( ${LIB_NAME} LINK_PRIVATE
  ${IDC_LIB_NAME}
  ${IDL_LIBRARIES}
  ${LIBXML2_LIBRARIES} 
  ${NETCDF_LIBRARIES}
  ${UDUNITS_LIBRARIES} 
  client-static
)

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME putdata
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  putdata.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )
