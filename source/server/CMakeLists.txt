cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( PostgreSQL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( OpenSSL REQUIRED )

include( modules )
find_modules()

add_subdirectory( modules )

########################################################################################################################
# Sources

set( SRC_FILES
  idamServer.c
  idamserverGetData.c
  freeIdamPut.c
  idamLegacyServer.c
  idamServerPlugin.c
  idamServerLegacyPlugin.c
  makeServerRequestBlock.c
  idamServer.c
  sleepServer.c
  writer.c
  CreateXDRStream.c
  mastArchiveFilePath.c
  closeServerSockets.c
  dumpFile.c
  idamserverSubsetData.c
  applyXML.c
  manageFiles.c
  serverProcessing.c
  sqllib.c
  managePluginFiles.c
  )

if( NOT FAT_BUILD )
  list( APPEND SRC_FILES
    idamServerStartup.c
    getServerEnvironment.c
    )
endif()

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

add_library( server-objects OBJECT
  ${SRC_FILES}
)

add_library( server-shared SHARED
  $<TARGET_OBJECTS:server-objects>
  $<TARGET_OBJECTS:clientserver-server>
  $<TARGET_OBJECTS:serverlogging>
  $<TARGET_OBJECTS:structures>
  $<TARGET_OBJECTS:security>
  )

add_library( server-static STATIC
  $<TARGET_OBJECTS:server-objects>
  $<TARGET_OBJECTS:clientserver-server>
  $<TARGET_OBJECTS:serverlogging>
  $<TARGET_OBJECTS:structures>
  $<TARGET_OBJECTS:security>
  )

if( NOT FAT_BUILD )
  add_executable( server-exe idamServerMain.c )
endif()

########################################################################################################################
# Add libraries

target_link_libraries( server-shared LINK_PUBLIC
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
  )

link_modules( server-shared )

target_link_libraries( server-static LINK_PUBLIC
  client-shared
  ${OPENSSL_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
  )

link_modules( server-static )

if( NOT FAT_BUILD )
  # Executable
  target_link_libraries( server-exe LINK_PUBLIC
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    c
    m
    dl
    server-shared
    client-shared
    ${OPENSSL_LIBRARIES}
    )

  if( NOT APPLE )
    target_link_libraries( server-exe LINK_PUBLIC gcc_s )
  endif()
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

if( NOT FAT_BUILD )
  set_target_properties( server-exe
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_server
  )

  install( TARGETS server-exe
    DESTINATION bin
  )
endif()

########################################################################################################################
# Configure files

string( REPLACE ";" ":" EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS}" )
configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamserver.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/idamserver.sh"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamserver.cfg.in"
  "${CMAKE_CURRENT_BINARY_DIR}/idamserver.cfg"
  @ONLY
)

set( USER $ENV{USER} )
configure_file(
  "${CMAKE_SOURCE_DIR}/etc/xinetd.conf.in"
  "${CMAKE_CURRENT_BINARY_DIR}/xinetd.conf"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/rc.idam.in"
  "${CMAKE_CURRENT_BINARY_DIR}/rc.idam"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/README.md.in"
  "${CMAKE_CURRENT_BINARY_DIR}/README.md"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/etc/idamgenstruct.conf"
  "${CMAKE_CURRENT_BINARY_DIR}/idamgenstruct.conf"
  COPYONLY
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xinetd.conf
    ${CMAKE_CURRENT_BINARY_DIR}/README.md
    ${CMAKE_CURRENT_BINARY_DIR}/idamserver.cfg
  DESTINATION etc
)

install(
  DIRECTORY
    ${CMAKE_SOURCE_DIR}/etc/machine.d
  DESTINATION etc
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/idamserver.sh
    ${CMAKE_CURRENT_BINARY_DIR}/rc.idam
  DESTINATION etc
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/plugins.d
  DESTINATION etc
)

install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/idamgenstruct.conf
  DESTINATION bin/plugins
)