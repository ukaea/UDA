########################################################################################################################
# Dependencies

find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )

########################################################################################################################
# Sources

set( SRC_FILES
  applyXML.cpp
  closeServerSockets.cpp
  dumpFile.cpp
  freeIdamPut.cpp
  getPluginAddress.cpp
  getServerEnvironment.cpp
  initPluginList.cpp
  makeServerRequestBlock.cpp
  mastArchiveFilePath.cpp
  serverGetData.cpp
  serverLegacyPlugin.cpp
  serverPlugin.cpp
  serverProcessing.cpp
  serverStartup.cpp
  serverSubsetData.cpp
  writer.cpp
  sqllib.cpp
)

set( HEADER_FILES
  applyXML.h
  closeServerSockets.h
  createXDRStream.h
  dumpFile.h
  freeIdamPut.h
  getPluginAddress.h
  getServerEnvironment.h
  initPluginList.h
  makeServerRequestBlock.h
  mastArchiveFilePath.h
  serverGetData.h
  serverLegacyPlugin.h
  serverPlugin.h
  serverProcessing.h
  serverStartup.h
  serverSubsetData.h
  sleepServer.h
  udaLegacyServer.h
  udaServer.h
  writer.h
  sqllib.h
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
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
  ${HEADER_FILES}
)

add_library( fatserver-objects OBJECT
  fatServer.cpp
  ${SRC_FILES}
  ${HEADER_FILES}
)

target_compile_definitions( fatserver-objects PRIVATE -DFATCLIENT )
set( SERVER_OBJ
  $<TARGET_OBJECTS:server-objects>
  $<TARGET_OBJECTS:clientserver-server-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:authentication-server-objects>
)

add_library( server-shared SHARED ${SERVER_OBJ} )
add_library( server-static STATIC ${SERVER_OBJ} )

add_executable( server-exe serverMain.cpp )

########################################################################################################################
# Add libraries

target_link_libraries( server-shared PRIVATE
  client-shared
  plugins-static
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
  dl
)

target_link_libraries( server-static PRIVATE
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
  dl
)

# Executable
target_link_libraries( server-exe PRIVATE
  client-shared
  server-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${CACHE_LIBRARIES}
  c
  m
  dl
)

if( NOT APPLE )
  target_link_libraries( server-exe PRIVATE gcc_s )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_server
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties( server-static
  PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME}_server
)

install(
  TARGETS server-shared server-static
  DESTINATION lib
)

########################################################################################################################
# Set executable properties

set_target_properties( server-exe
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_server
)

install( TARGETS server-exe
  DESTINATION bin
)

########################################################################################################################
# Configure files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )

set( ETC_FILES
  udaserver.sh
  udaserver.cfg
  xinetd.conf
  rc.uda
  README.md
)

set( PKGCONFIG_FILES
  uda-client.pc
  uda-fat-client.pc
  uda-cpp.pc
  uda-fat-cpp.pc
  uda-plugins.pc
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

foreach( PKGCONFIG_FILE ${PKGCONFIG_FILES} )
  configure_file(
    "${CMAKE_SOURCE_DIR}/source/etc/${PKGCONFIG_FILE}.in"
    "${CMAKE_BINARY_DIR}/lib/pkgconfig/${PKGCONFIG_FILE}"
    @ONLY
  )
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
