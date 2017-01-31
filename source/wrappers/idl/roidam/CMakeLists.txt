cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL REQUIRED )
find_package( IDL REQUIRED )
find_package( LibXml2 REQUIRED )

set( LIB_NAME roidam )

set( SRC_FILES roidam.c roidamlib.c getExpDateTime.c getLastShot.c getLatestSourcePass.c getSourcePath.c )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${LIBXML2_INCLUDE_DIR}
  ${IDL_INCLUDES}
  ${PostgreSQL_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} 
 ${IDL_LIBRARIES} ${LIBXML2_LIBRARIES} ${PostgreSQL_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME roidam
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  roidam.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )
