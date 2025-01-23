find_package( spdlog REQUIRED )

set( SRCS
  udaGetAPI.cpp
  accAPI.cpp
  udaPutAPI.cpp
)

add_library( c_api OBJECT ${SRCS} )
target_include_directories( c_api PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( c_api PRIVATE logging clientserver client structures common )