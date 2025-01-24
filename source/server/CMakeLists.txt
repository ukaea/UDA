########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( Boost REQUIRED )

set( LIBS LibXml2::LibXml2 OpenSSL::SSL OpenSSL::Crypto fmt::fmt )

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
  applyXML.cpp
  closeServerSockets.cpp
  getPluginAddress.cpp
  initPluginList.cpp
  makeServerRequestBlock.cpp
  serverGetData.cpp
  serverLegacyPlugin.cpp
  serverPlugin.cpp
  serverProcessing.cpp
  serverStartup.cpp
  serverSubsetData.cpp
  writer.cpp
  createXDRStream.cpp
  server_config.cpp
  plugin_api.cpp
)

add_definitions( -DSERVERBUILD )

########################################################################################################################
# Targets

add_library( server-objects OBJECT
  udaServer.cpp
  udaLegacyServer.cpp
  createXDRStream.cpp
  sleepServer.cpp
  ${SRC_FILES}
)
target_link_libraries( server-objects PRIVATE ${LIBS} )
target_include_directories( server-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)
target_compile_definitions(
  server-objects PRIVATE
)

add_library( fatserver-objects OBJECT
  fatServer.cpp
  ${SRC_FILES}
  ${HEADER_FILES}
)
target_link_libraries( fatserver-objects PRIVATE ${LIBS} )
target_include_directories( fatserver-objects PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
  ${CMAKE_BINARY_DIR}/include
)

target_compile_definitions( fatserver-objects PRIVATE -DFATCLIENT )

set( OBJECT_LIBS
  server-objects
  clientserver-server-objects
  logging-server-objects
  structures-server-objects
  authentication-server-objects
  cache-objects
)

add_library( server-static STATIC )
if( BUILD_SHARED_LIBS )
  add_library( server-shared SHARED )
endif()

target_link_libraries( server-static PRIVATE ${LIBS} ${OBJECT_LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( server-shared PRIVATE ${LIBS} ${OBJECT_LIBS} )
endif()

set( EXE_FLAG )
if( WIN32 OR MINGW )
  set( EXE_FLAG WIN32 )
endif()

add_executable( server-exe ${EXE_FLAG} serverMain.cpp )

########################################################################################################################
# Add libraries

set( CLIENT_STATIC )
if( BUILD_SHARED_LIBS )
  set( CLIENT_STATIC client-shared )
else()
  set( CLIENT_STATIC client-static )
endif()

target_link_libraries( server-static PRIVATE
  ${CLIENT_STATIC}
  ${CACHE_LIBRARIES}
)

if( BUILD_SHARED_LIBS )
  target_link_libraries( server-shared PRIVATE
    client-shared
    ${CACHE_LIBRARIES}
  )
endif()

# Executable
set( SERVER_LINKING )
if( BUILD_SHARED_LIBS )
  set( SERVER_LINKING client-shared server-shared )
else()
  set( SERVER_LINKING client-static server-static )
endif()

target_link_libraries( server-exe PRIVATE
  ${SERVER_LINKING}
  ${LIBS}
)
target_include_directories( server-exe PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/include
)

if( NOT WIN32 AND NOT MINGW )
  target_link_libraries( server-exe LINK_PRIVATE c )
endif()

if( NOT APPLE AND ( NOT WIN32 OR ( WIN32 AND MINGW ) ) )
  target_link_libraries( server-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server
)

install(
  TARGETS server-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( server-shared
      PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}_server
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  if( WIN32 )
    set_target_properties( server-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

  install(
    TARGETS server-shared
    DESTINATION lib
  )
endif()

########################################################################################################################
# Set executable properties

set_target_properties( server-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server
)

if( WIN32 )
  set_target_properties( server-exe
    PROPERTIES
     COMPILE_FLAGS -DNOIMPLIB
  )
endif()

install( TARGETS server-exe
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
  uda.socket
  uda@.service
  uda.plist
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

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/server
)
