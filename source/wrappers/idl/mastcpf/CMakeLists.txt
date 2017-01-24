cmake_minimum_required( VERSION 2.8 )

find_package( PostgreSQL REQUIRED )
find_package( IDL REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../.. )

set( LIB_NAME mastcpf )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SRC_FILES mastCPF.c )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${SRC_DIR}/client
  ${IDL_INCLUDES}
  ${PostgreSQL_INCLUDE_DIRS}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

link_directories( ${LIB_DIR} )
target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} 
 ${IDL_LIBRARIES} ${PostgreSQL_LIBRARIES} )


set_target_properties( ${LIB_NAME}
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME mastCPF
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)

install( TARGETS ${LIB_NAME}
  DESTINATION dlm
)

set( INSTALL_FILES
  mastCPF.dlm
)

install( FILES ${INSTALL_FILES}
  DESTINATION dlm )
