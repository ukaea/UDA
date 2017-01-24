cmake_minimum_required( VERSION 2.8 )

set( PHPCPP_INCLUDE_DIR /home/jholloc/include )
set( PHPCPP_LIBRARIES /home/jholloc/lib/libphpcpp.so )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME idamphp )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SRC_FILES php_idamgetapi.cpp )

include_directories(
  ${SRC_DIR}/include
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${SRC_DIR}/client
  ${PHPCPP_INCLUDE_DIR}
)

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${LIB_DIR}/libidam.so ${PHPCPP_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)
