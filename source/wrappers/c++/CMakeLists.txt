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
  signal.cpp
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${Boost_INCLUDE_DIR}
)

add_definitions( -D__USE_XOPEN2K8 )

include_directories( SYSTEM "${Boost_INCLUDE_DIR}" )

add_library( cpp-objects OBJECT ${SRC_FILES} )

add_library( cpp-shared SHARED $<TARGET_OBJECTS:cpp-objects> )
add_library( cpp-static STATIC $<TARGET_OBJECTS:cpp-objects> )

target_link_libraries( cpp-shared LINK_PRIVATE client-shared ${Boost_LIBRARIES} )
target_link_libraries( cpp-static LINK_PRIVATE client-static ${Boost_LIBRARIES} )

if( NOT CLIENT_ONLY )
  add_library( fatcpp-objects OBJECT ${SRC_FILES} )
  target_compile_definitions( fatcpp-objects PRIVATE -DFATCLIENT )
  add_library( fatcpp-shared SHARED $<TARGET_OBJECTS:fatcpp-objects> )
  target_link_libraries( fatcpp-shared LINK_PRIVATE fatclient-shared ${Boost_LIBRARIES} )
endif()

set_target_properties( cpp-shared
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_cpp
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties( cpp-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_cpp
)

install( TARGETS cpp-shared cpp-static
  DESTINATION lib
)

if( NOT CLIENT_ONLY )
  set_target_properties( fatcpp-shared
    PROPERTIES
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )

  install( TARGETS fatcpp-shared
    DESTINATION lib
  )
endif()

set( INSTALL_FILES
  UDA.hpp
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
  DESTINATION include/uda/c++ )
