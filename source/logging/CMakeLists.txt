########################################################################################################################
# Dependencies

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET)
  if(TIRPC_FOUND)
    add_definitions(-D__TIRPC__)
    
      endif()
endif()

########################################################################################################################
# Sources

if( "${LOGGING_BACKEND}" STREQUAL "FILE" )
  set( SRC_FILES fileLogging.cpp )
elseif( "${LOGGING_BACKEND}" STREQUAL "SYSLOG" )
  set( SRC_FILES sysLogging.cpp )
else()
  message( FATAL_ERROR "Unknown logging backend ${LOGGING_BACKEND}" )
endif()

include_directories( ${CMAKE_SOURCE_DIR}/source )

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif(TIRPC_FOUND)
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

########################################################################################################################
# Targets

add_library( logging-client-objects OBJECT ${SRC_FILES} )

if( NOT CLIENT_ONLY )
  add_library( logging-server-objects OBJECT accessLog.cpp ${SRC_FILES} )
  target_compile_definitions( logging-server-objects PRIVATE -DSERVERBUILD )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/logging
)

if( MINGW )
  add_definitions( -Wno-error=pointer-sign )
endif()