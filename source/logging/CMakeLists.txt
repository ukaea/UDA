if( "${LOGGING_BACKEND}" STREQUAL "FILE" )
  set( SRC_FILES fileLogging.cpp )
elseif( "${LOGGING_BACKEND}" STREQUAL "SYSLOG" )
  set( SRC_FILES sysLogging.cpp )
else()
  message( FATAL_ERROR "Unknown logging backend ${LOGGING_BACKEND}" )
endif()

include_directories( ${CMAKE_SOURCE_DIR}/source )

add_library( logging-client-objects OBJECT ${SRC_FILES} )

if( NOT CLIENT_ONLY )
  add_library( logging-server-objects OBJECT accessLog.cpp ${SRC_FILES} )
  target_compile_definitions( logging-server-objects PRIVATE -DSERVERBUILD )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/logging
)
