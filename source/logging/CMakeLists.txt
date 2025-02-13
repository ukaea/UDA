find_package( fmt REQUIRED )
find_package( spdlog REQUIRED )

set( SRCS
  access_log.cpp
  logging.cpp
)

add_library( logging OBJECT ${SRCS} )
target_include_directories( logging PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/source ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( logging PUBLIC SPDLOG_COMPILED_LIB )
target_link_libraries( logging PRIVATE fmt::fmt spdlog::spdlog )
