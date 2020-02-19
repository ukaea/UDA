########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
if( NOT CLIENT_ONLY )
  find_package( LibXml2 REQUIRED )
endif()

if( NOT CLIENT_ONLY AND NOT NO_MODULES )
  find_package( PostgreSQL REQUIRED )
else()
  add_definitions( -DNOTGENERICENABLED )
endif()

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  if( NOT MINGW )
    find_package( dlfcn-win32 CONFIG REQUIRED )
  endif()
endif()

add_definitions( -DNO_GSL_LIB )

########################################################################################################################
# Sources

set( SRC_FILES
  udaGetAPI.cpp
  clientAPI.cpp
  clientMDS.cpp
  accAPI.cpp
  startup.cpp
  closedown.cpp
  connection.cpp
  clientXDRStream.cpp
  updateSelectParms.cpp
  getEnvironment.cpp
  generateErrors.cpp
  makeClientRequestBlock.cpp
  udaPutAPI.cpp
  udaClient.cpp
  udaClientHostList.cpp
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

if( NOT CLIENT_ONLY )
  include_directories( ${LIBXML2_INCLUDE_DIR} )
endif()

add_library( client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )

  target_compile_definitions( fatclient-objects PRIVATE -DFATCLIENT )
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
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-server-objects>
)

add_library( client-static STATIC ${CLIENT_OBJS} )
if( BUILD_SHARED_LIBS )
  add_library( client-shared SHARED ${CLIENT_OBJS} )
endif()

set( CLIENT_LINK_LIBS ${OPENSSL_LIBRARIES} ${CACHE_LIBRARIES} )
if( NOT CLIENT_ONLY )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ${LIBXML2_LIBRARIES} )
  if( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} iconv lzma z )
  endif()
endif()

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ws2_32 ${XDR_LIBRARIES} )
  if ( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dl stdc++ )
  else()
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dlfcn-win32::dl )
  endif()
endif()

target_link_libraries( client-static PRIVATE ${CLIENT_LINK_LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( client-shared PRIVATE ${CLIENT_LINK_LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient-static PRIVATE plugins-static ${CLIENT_LINK_LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient-shared PRIVATE plugins-shared ${CLIENT_LINK_LIBS} )
  endif()
endif()

set_target_properties( client-static
  PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}_client
)

install( TARGETS client-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  set_target_properties( client-shared
    PROPERTIES
      OUTPUT_NAME ${PROJECT_NAME}_client
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
      SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  
  install( TARGETS client-shared
    DESTINATION lib
  )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/client
)

if( NOT CLIENT_ONLY )
  set_target_properties( fatclient-static
    PROPERTIES
    OUTPUT_NAME fat${PROJECT_NAME}_client
  )

  install( TARGETS fatclient-static
    DESTINATION lib
  )
  
  if( BUILD_SHARED_LIBS )
    set_target_properties( fatclient-shared
      PROPERTIES
        OUTPUT_NAME fat${PROJECT_NAME}_client
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    install( TARGETS fatclient-shared
      DESTINATION lib
  )
  endif()
endif()
