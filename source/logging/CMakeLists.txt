cmake_minimum_required( VERSION 2.8 )

set( SRC_FILES
  logging.c
  accessLog.c
)

include_directories( ${CMAKE_SOURCE_DIR}/source )

add_library( logging OBJECT logging.c )
add_library( serverlogging OBJECT ${SRC_FILES} )