cmake_minimum_required( VERSION 2.8 )

find_package( IDL REQUIRED )
find_package( LibXml2 REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../.. )

set( LIB_NAME putdata )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SRC_FILES
  putdata.c
)

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${SRC_DIR}/client
  ${LIBXML2_INCLUDE_DIR}
  ${IDL_INCLUDES}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

link_directories( ${LIB_DIR} )
target_link_libraries( ${LIB_NAME} LINK_PUBLIC
  ${IDC_LIB_NAME}
  ${IDL_LIBRARIES}
  ${LIBXML2_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME putdata
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  putdata.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )
