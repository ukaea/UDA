cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( PostgreSQL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( GCrypt REQUIRED )
find_package( KSBA REQUIRED )

include( modules )
find_modules()

add_subdirectory( modules )

########################################################################################################################
# Sources

set( SRC_FILES
  udaServer.c
  serverGetData.c
  freeIdamPut.c
  udaLegacyServer.c
  serverPlugin.c
  serverLegacyPlugin.c
  makeServerRequestBlock.c
  udaServer.c
  sleepServer.c
  writer.c
  createXDRStream.c
  mastArchiveFilePath.c
  closeServerSockets.c
  dumpFile.c
  serverSubsetData.c
  applyXML.c
  manageFiles.c
  serverProcessing.c
  sqllib.c
  managePluginFiles.c
  serverStartup.c
  getServerEnvironment.c
  getPluginAddress.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PostgreSQL_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
)

add_definitions( -DSERVERBUILD )

########################################################################################################################
# Targets

link_directories( ${PostgreSQL_LIBRARY_DIRS} )

add_library( server-objects OBJECT ${SRC_FILES} )
add_library( fatserver-objects OBJECT ${SRC_FILES} )

target_compile_definitions( fatserver-objects PRIVATE -DFATCLIENT )

set( SERVER_OBJ
  $<TARGET_OBJECTS:server-objects>
  $<TARGET_OBJECTS:clientserver-server-objects>
  $<TARGET_OBJECTS:logging-server-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:security-server-objects>
  $<TARGET_OBJECTS:modules-objects>
)

add_library( server-shared SHARED ${SERVER_OBJ} )
add_library( server-static STATIC ${SERVER_OBJ} )

add_executable( server-exe serverMain.c )

########################################################################################################################
# Add libraries

target_link_libraries( server-shared LINK_PRIVATE
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
  ${CACHE_LIBRARIES}
)

link_modules( server-shared )

target_link_libraries( server-static LINK_PRIVATE
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
  ${CACHE_LIBRARIES}
)

link_modules( server-static )

# Executable
target_link_libraries( server-exe LINK_PRIVATE
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
  c
  m
  dl
  server-shared
  client-shared
  ${OPENSSL_LIBRARIES}
  ${GCRYPT_LIBRARIES}
  ${KSBA_LIBRARIES}
)

if( NOT APPLE )
  target_link_libraries( server-exe LINK_PRIVATE gcc_s )
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
  uda-client.pc
  uda-fat-client.pc
  uda-cpp.pc
  uda-fat-cpp.pc
  README.md
)

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
    ${CMAKE_SOURCE_DIR}/source/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_BINARY_DIR}/etc/udagenstruct.conf
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
