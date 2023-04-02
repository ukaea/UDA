########################################################################################################################
# Dependencies

find_package( Boost REQUIRED )

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

add_library( logging-client-objects OBJECT  )
add_library( uda::logging-client-objects ALIAS logging-client-objects )

target_sources( logging-client-objects PRIVATE ${SRC_FILES} )
target_compile_features( logging-client-objects PRIVATE cxx_std_11 )
target_link_libraries( logging-client-objects PUBLIC Boost::boost )
target_include_directories( logging-client-objects PRIVATE ${PROJECT_SOURCE_DIR}/source )

if( NOT CLIENT_ONLY )
  add_library( logging-server-objects OBJECT accessLog.cpp ${SRC_FILES} )
  add_library( uda::logging-server-objects ALIAS logging-server-objects )

  target_sources( logging-server-objects PRIVATE ${SRC_FILES} )
  target_sources( logging-server-objects PRIVATE accessLog.cpp )
  target_compile_features( logging-server-objects PRIVATE cxx_std_11 )
  target_link_libraries( logging-server-objects PUBLIC Boost::boost )
  target_include_directories( logging-server-objects PRIVATE ${PROJECT_SOURCE_DIR}/source )
endif()

if( MINGW )
  target_compile_definitions( logging-client-objects PRIVATE -Wno-error=pointer-sign )
  if( NOT CLIENT_ONLY )
    target_compile_definitions( logging-server-objects PRIVATE -Wno-error=pointer-sign )
  endif()
endif()
