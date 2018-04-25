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
  udaClientHostList.c
)

set( HEADER_FILES
  accAPI.h
  clientAPI.h
  clientMDS.h
  clientXDRStream.h
  closedown.h
  connection.h
  generateErrors.h
  getEnvironment.h
  makeClientRequestBlock.h
  startup.h
  udaClient.h
  udaGetAPI.h
  udaPutAPI.h
  updateSelectParms.h
  udaClientHostList.h
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
)

add_library( client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )

  target_compile_definitions( fatclient-objects PRIVATE
    -DFATCLIENT
    -DNOIDAPLUGIN
    -DNOPPFPLUGIN
    -DNOJPFPLUGIN
    -DNOHDF5PLUGIN
    -DNONETCDFPLUGIN
    -DNOMDSPLUSPLUGIN
    -DNOTGENERICENABLED
  )
  target_include_directories( fatclient-objects PRIVATE ${PostgreSQL_INCLUDE_DIRS} )
endif()

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

set( FATCLIENT_OBJS
  $<TARGET_OBJECTS:fatclient-objects>
  $<TARGET_OBJECTS:fatserver-objects>
  $<TARGET_OBJECTS:fatclientserver-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:fatmodules-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-server-objects>
)

add_library( client-shared SHARED ${CLIENT_OBJS} )
add_library( client-static STATIC ${CLIENT_OBJS} )

set( CLIENT_LINK_LIBS ${OPENSSL_LIBRARIES} ${CACHE_LIBRARIES} )
if( NOT CLIENT_ONLY )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ${LIBXML2_LIBRARIES} plugins-static )
endif()

if( WIN32 )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ws2_32 xdr )
endif()

target_link_libraries( client-shared PRIVATE ${CLIENT_LINK_LIBS} )
target_link_libraries( client-static PRIVATE ${CLIENT_LINK_LIBS} )

if( NOT CLIENT_ONLY )
  add_library( fatclient-shared SHARED ${FATCLIENT_OBJS} )
  add_library( fatclient-static STATIC ${FATCLIENT_OBJS} )

  target_link_libraries( fatclient-shared PRIVATE ${CLIENT_LINK_LIBS} ${PostgreSQL_LIBRARY} )
  target_link_libraries( fatclient-static PRIVATE ${CLIENT_LINK_LIBS} ${PostgreSQL_LIBRARY} )
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
