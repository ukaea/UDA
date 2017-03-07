cmake_minimum_required( VERSION 2.8 )

########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
if( NOT CLIENT_ONLY )
  find_package( LibXml2 REQUIRED )
  find_package( PostgreSQL REQUIRED )
endif()

add_definitions( -DNO_GSL_LIB )

########################################################################################################################
# Sources

set( SRC_FILES
  udaGetAPI.c
  clientAPI.c
  clientMDS.c
  accAPI.c
  startup.c
  closedown.c
  connection.c
  clientXDRStream.c
  updateSelectParms.c
  getEnvironment.c
  generateErrors.c
  makeClientRequestBlock.c
  udaPutAPI.c
  udaClient.c
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
)

add_library( client-objects OBJECT ${SRC_FILES} )

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )

  target_compile_definitions( fatclient-objects PRIVATE -DFATCLIENT )
  target_include_directories( fatclient-objects PRIVATE ${PostgreSQL_INCLUDE_DIRS} )
endif()

set( CLIENT_SRCS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache>
  $<TARGET_OBJECTS:logging>
  $<TARGET_OBJECTS:structures-client>
  )

set( FATCLIENT_SRCS
  $<TARGET_OBJECTS:fatclient-objects>
  $<TARGET_OBJECTS:fatserver-objects>
  $<TARGET_OBJECTS:fatclientserver-objects>
  $<TARGET_OBJECTS:serverlogging>
  $<TARGET_OBJECTS:structures-server>
  $<TARGET_OBJECTS:security>
  $<TARGET_OBJECTS:fatmodules-objects>
  $<TARGET_OBJECTS:cache>
  $<TARGET_OBJECTS:serverlogging>
  )

add_library( client-shared SHARED ${CLIENT_SRCS} )
add_library( client-static STATIC ${CLIENT_SRCS} )

if( NOT CLIENT_ONLY )
  add_library( fatclient-shared SHARED ${FATCLIENT_SRCS} )
  add_library( fatclient-static STATIC ${FATCLIENT_SRCS} )
endif()

set_target_properties( client-shared
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties( client-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client
)

install( TARGETS client-shared client-static
  DESTINATION lib
)

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/client
)

if( NOT CLIENT_ONLY )
  set_target_properties( fatclient-shared
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    )

  set_target_properties( fatclient-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client
    )

  install( TARGETS fatclient-shared fatclient-static
    DESTINATION lib
    )
endif()
