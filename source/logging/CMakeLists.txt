cmake_minimum_required( VERSION 2.8 )

set( SRC_FILES
  logging.c
  accessLog.c
)

include_directories( ${CMAKE_SOURCE_DIR}/source )

add_library( logging-client-objects OBJECT logging.c )
add_library( logging-server-objects OBJECT ${SRC_FILES} )

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/logging
)