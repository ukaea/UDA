cmake_minimum_required( VERSION 2.8 )

find_package( Boost REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME idamcpp )
set( MAJOR_VERSION 1 )
set( MINOR_VERSION 8 )

set( SOURCES array.cpp client.cpp data.cpp dim.cpp result.cpp scalar.cpp string.cpp structdata.cpp treenode.cpp vector.cpp )
set( HEADERS Idam.hpp array.hpp client.hpp data.hpp dim.hpp result.hpp scalar.hpp string.hpp structdata.hpp treenode.hpp vector.hpp )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall" )

include_directories(
  ${SRC_DIR}
  ${SRC_DIR}/include
  ${SRC_DIR}/client
  ${SRC_DIR}/clientserver
  ${SRC_DIR}/structures
  ${Boost_INCLUDE_DIR}
)

link_directories(
  ${SRC_DIR}/../build/lib
)

add_library( ${LIB_NAME} SHARED ${SOURCES} ${HEADERS} )

include_directories(SYSTEM "${Boost_INCLUDE_DIR}")

if( FAT_BUILD )
  target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME} ${Boost_LIBRARIES} )
else()
  target_link_libraries( ${LIB_NAME} LINK_PUBLIC ${IDC_LIB_NAME}-static ${Boost_LIBRARIES} )
endif()

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${MAJOR_VERSION}.${MINOR_VERSION}
    SOVERSION ${MAJOR_VERSION}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)

set( INSTALL_FILES
  ${HEADERS}
)

install( FILES ${INSTALL_FILES}
  DESTINATION include/idam/c++ )
