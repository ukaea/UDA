find_package( Boost REQUIRED headers )
find_package( fmt REQUIRED )

set( SRCS
  config.cpp
)

add_library( config OBJECT ${SRCS} )
target_include_directories( config PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( config PRIVATE ${CMAKE_SOURCE_DIR}/extlib )
target_link_libraries( config PUBLIC Boost::boost fmt::fmt )
# target_include_directories( config PUBLIC Boost::boost )
