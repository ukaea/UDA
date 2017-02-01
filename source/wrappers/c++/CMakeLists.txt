cmake_minimum_required( VERSION 2.8 )

find_package( Boost REQUIRED )

set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

set( LIB_NAME ${PROJECT_NAME}_cpp )

set( SRC_FILES
  array.cpp
  client.cpp
  data.cpp
  dim.cpp
  result.cpp
  scalar.cpp
  string.cpp
  structdata.cpp
  treenode.cpp
  vector.cpp
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${Boost_INCLUDE_DIR}
)

add_definitions( -D__USE_XOPEN2K8 )

add_library( ${LIB_NAME} SHARED ${SRC_FILES} )

include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

target_link_libraries( ${LIB_NAME} LINK_PUBLIC client-static ${Boost_LIBRARIES} )

set_target_properties( ${LIB_NAME}
  PROPERTIES
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install( TARGETS ${LIB_NAME}
  DESTINATION lib
)

set( INSTALL_FILES
  array.hpp
  client.hpp
  data.hpp
  dim.hpp
  result.hpp
  scalar.hpp
  string.hpp
  structdata.hpp
  treenode.hpp
  vector.hpp
)

install( FILES ${INSTALL_FILES}
  DESTINATION include/idam/c++ )
