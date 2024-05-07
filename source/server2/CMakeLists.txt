########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( spdlog REQUIRED )
find_package( fmt REQUIRED )

set( LIBS LibXml2::LibXml2 OpenSSL::SSL OpenSSL::Crypto spdlog::spdlog fmt::fmt )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
  list( APPEND LIBS XDR::XDR )
  list( APPEND LIBS dlfcn-win32::dl )
  list( APPEND LIBS ws2_32 )
  if( MINGW )
    list( APPEND LIBS stdc++ )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    list( APPEND LIBS TIRPC::TIRPC )
  endif()
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  apply_XML.cpp
  get_data.cpp
  get_plugin_address.cpp
  make_server_request_block.cpp
  plugins.cpp
  server.cpp
  server_environment.cpp
  server_exceptions.cpp
  server_plugin.cpp
  server_processing.cpp
  server_subset_data.cpp
  xdr_protocol.cpp
)

add_definitions( -DSERVERBUILD )

########################################################################################################################
# Targets

add_library( server2-objects OBJECT
  ${SRC_FILES}
)
target_link_libraries( server2-objects PRIVATE ${LIBS} )
target_include_directories( server2-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

set( OBJECT_LIBS
  server2-objects
  clientserver-server-objects
  logging-server-objects
  structures-server-objects
  authentication-server-objects
  cache-objects
)

add_library( server2-static STATIC )
if( BUILD_SHARED_LIBS )
  add_library( server2-shared SHARED )
endif()

target_link_libraries( server2-static PRIVATE ${LIBS} ${OBJECT_LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server2-shared PRIVATE ${LIBS} ${OBJECT_LIBS} )
endif()

set( EXE_FLAG )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
endif()

add_executable( server2-exe ${EXE_FLAG} server_main.cpp)

########################################################################################################################
# Add libraries

set( CLIENT_STATIC )
if( BUILD_SHARED_LIBS )
  set( CLIENT_STATIC client-shared )
else()
  set( CLIENT_STATIC client-static )
endif()

target_link_libraries( server2-static PRIVATE
  ${CLIENT_STATIC}
  ${CACHE_LIBRARIES}
)

if( BUILD_SHARED_LIBS )
  target_link_libraries( server2-shared PRIVATE
    client-shared
    plugins-shared
    ${CACHE_LIBRARIES}
  )
endif()

# Executable
set( SERVER_LINKING )
if( BUILD_SHARED_LIBS )
  set( SERVER_LINKING client-shared server2-shared plugins-shared )
else()
  set( SERVER_LINKING client-static server2-static plugins-static )
endif()

target_link_libraries( server2-exe PRIVATE
  ${SERVER_LINKING}
  ${LIBS}
)
target_include_directories( server2-exe PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server2-exe LINK_PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server2-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server2-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server2
)

install(
  TARGETS server2-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( server2-shared
      PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}_server2
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( server2-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install(
    TARGETS server2-shared
    DESTINATION lib
  )
endif()

########################################################################################################################
# Set executable properties

set_target_properties( server2-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server2${EXE_EXT}
)

if( WIN32 )
  set_target_properties( server2-exe
    PROPERTIES
     COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install( TARGETS server2-exe
  DESTINATION bin
)

########################################################################################################################
# Configure server files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )

set( ETC_FILES
  udaserver.sh
  udaserver.cfg
  xinetd.conf
  rc.uda
  README.md
)

set( GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf )

foreach( ETC_FILE ${ETC_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${ETC_FILE}.in"
    "${CMAKE_BINARY_DIR}/etc/${ETC_FILE}"
    @ONLY
  )
  list( APPEND GEN_ETC_FILES ${CMAKE_BINARY_DIR}/etc/${ETC_FILE} )
endforeach()

configure_file(
  "${CMAKE_SOURCE_DIR}/source/etc/udagenstruct.conf"
  "${CMAKE_BINARY_DIR}/etc/udagenstruct.conf"
  COPYONLY
)

install(
  FILES
    ${GEN_ETC_FILES}
  DESTINATION etc
)

install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/lib/pkgconfig
  DESTINATION lib
)

install(
  DIRECTORY
    ${CMAKE_SOURCE_DIR}/source/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udaserver.sh
    ${CMAKE_BINARY_DIR}/etc/rc.uda
  DESTINATION etc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY
    ${CMAKE_BINARY_DIR}/etc/plugins.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
  DESTINATION etc
)
