########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( Boost REQUIRED )

set( LIBS LibXml2::LibXml2 Boost::boost )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  list( APPEND LIBS XDR::XDR )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
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

########################################################################################################################
# Targets

add_library( logging-client-objects OBJECT ${SRC_FILES} )

if( NOT CLIENT_ONLY )
  add_library( logging-server-objects OBJECT accessLog.cpp ${SRC_FILES} )
  target_compile_definitions( logging-server-objects PRIVATE -DSERVERBUILD )
endif()

target_link_libraries( logging-client-objects PUBLIC ${LIBS} )
target_link_libraries( logging-server-objects PUBLIC ${LIBS} )

target_include_directories( logging-client-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)
target_include_directories( logging-server-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( MINGW )
  target_compile_options( logging-client-objects PRIVATE -Wno-error=pointer-sign )
  target_compile_options( logging-server-objects PRIVATE -Wno-error=pointer-sign )
endif()
