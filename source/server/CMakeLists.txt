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

add_definitions( -DCOMPRESSOFF )
add_definitions( -DOSTYPE=linux )
#add_definitions( -DMDSSANDBOX )
add_definitions( -DISCOMPLIANT )
add_definitions( -D_GNU_SOURCE )
#add_definitions( -DIDAMPLUGIN )
add_definitions( -DFILELISTTEST )
add_definitions( -DSINGLEPACKET )
add_definitions( -DGENERALSTRUCTS )
add_definitions( -DDGMJUL14 )
add_definitions( -DCLEANNAMESPACE25SEP14 )
add_definitions( -DSERVERBUILD )

########################################################################################################################
# Targets

link_directories( ${PostgreSQL_LIBRARY_DIRS} )

if( NOT FAT_BUILD )
  add_library( server SHARED
    ${SRC_FILES}
    $<TARGET_OBJECTS:clientserver-server>
    $<TARGET_OBJECTS:serverlogging>
    $<TARGET_OBJECTS:structures>
    $<TARGET_OBJECTS:security>
    )
  add_executable( server.x idamServerMain.c )
else()
  add_library( server OBJECT ${SRC_FILES} )
endif()

########################################################################################################################
# Add libraries

target_link_libraries( server LINK_PUBLIC
  ${LIBXML2_LIBRARIES}
  ${PostgreSQL_LIBRARY}
)

if( NOT FAT_BUILD )
  target_link_libraries( server LINK_PUBLIC client-shared ${OPENSSL_LIBRARIES} )

  # Executable
  target_link_libraries( server.x LINK_PUBLIC
    ${LIBXML2_LIBRARIES}
    ${PostgreSQL_LIBRARY}
    c
    m
    dl
    server
    client-shared
    ${OPENSSL_LIBRARIES}
    )
  if( NOT APPLE )
    target_link_libraries( server.x LINK_PUBLIC gcc_s )
  endif()

  link_modules( server )
endif()

########################################################################################################################
# Set library properties

set_target_properties( server
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_server
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )

if( NOT FAT_BUILD )
  install(
    TARGETS server
    DESTINATION lib
  )

########################################################################################################################
# Set executable properties

  set_target_properties( server.x
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_server
  )

  install( TARGETS server.x
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