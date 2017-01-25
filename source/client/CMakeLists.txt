cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
if( FAT_BUILD )
  find_package( LibXml2 REQUIRED )
  find_package( PostgreSQL REQUIRED )
endif()

add_definitions( -DCLEANNAMESPACE25SEP14 -DOSTYPE=linux )
if( NOT FAT_BUILD )
  add_definitions( -DCLIENTBUILD )
endif()

########################################################################################################################
# Sources

set( SRC_FILES
  accAPI_C.c
  IdamAPI.c
  ClientAPI.c
  ClientMDS.c
  accAPI_C.c
  accAPI_CL.c
  accAPI_F.c
  accAPI_FL.c
  accAPI_Java.c
  accAPI_XML.c
  accAPI_Gen.c
  startup.c
  closedown.c
  idamCreateConnection.c
  createClientXDRStream.c
  UpdateSelectParms.c
  Writeout.c
  Readin.c
  getEnvironment.c
  generateErrors.c
  closeClientSockets.c
  makeClientRequestBlock.c
  idamPutAPI.c
  idam_client.c
  )

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
)

add_library( client OBJECT ${SRC_FILES} )

if( NOT FAT_BUILD )
  set( CLIENT_SRCS
    $<TARGET_OBJECTS:clientserver-client>
    $<TARGET_OBJECTS:client>
    $<TARGET_OBJECTS:cache>
    $<TARGET_OBJECTS:logging>
  )
else()
  set( CLIENT_SRCS
    $<TARGET_OBJECTS:server>
    $<TARGET_OBJECTS:modules>
    $<TARGET_OBJECTS:clientserver-server>
    $<TARGET_OBJECTS:serverlogging>
    $<TARGET_OBJECTS:structures>
    $<TARGET_OBJECTS:security>
    $<TARGET_OBJECTS:client>
    $<TARGET_OBJECTS:cache>
    $<TARGET_OBJECTS:serverlogging>
  )
endif()

add_library( client-shared SHARED ${CLIENT_SRCS} )
add_library( client-static STATIC ${CLIENT_SRCS} )

set_target_properties( client-shared
  PROPERTIES
    OUTPUT_NAME uda_client
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties( client-static
  PROPERTIES
    OUTPUT_NAME uda_client
)

install( TARGETS client-shared
  DESTINATION lib
)

install( TARGETS client-static
  DESTINATION lib
)
