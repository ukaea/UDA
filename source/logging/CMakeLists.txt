cmake_minimum_required( VERSION 2.8 )

set( SRC_FILES
  idamLog.c
  idamAccessLog.c
)

include_directories( ${CMAKE_SOURCE_DIR}/source )

add_library( logging OBJECT idamLog.c )
add_library( serverlogging OBJECT ${SRC_FILES} )