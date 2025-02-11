set( SRCS
  string_utils.cpp
)

add_library( common OBJECT ${SRCS} )
target_include_directories( common PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( common PRIVATE )