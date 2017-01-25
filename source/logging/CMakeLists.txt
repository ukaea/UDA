cmake_minimum_required( VERSION 2.8 )

set( LIB_NAME idamlog )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -std=gnu99" )
set( SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )

option( DEBUG "Debug info" ON )
if( DEBUG )
  add_definitions( -DDEBUG )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0" )
endif( DEBUG )

set( SRC_FILES
  idamLog.c
  idamAccessLog.c
)

include_directories( ${SRC_DIR} )

add_library( logging OBJECT idamLog.c )
add_library( serverlogging OBJECT ${SRC_FILES} )

