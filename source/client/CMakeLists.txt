########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
  find_package( dlfcn-win32 CONFIG REQUIRED )
  if( MINGW )
    find_package( ZLIB REQUIRED )
    find_package( Iconv REQUIRED )
    find_package( liblzma CONFIG REQUIRED )
  endif()
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

add_definitions( -DNOTGENERICENABLED )
add_definitions( -DNO_GSL_LIB )

if( LIBMEMCACHED_FOUND AND NOT NO_MEMCACHE )
  set( EXTRA_LD_LIBRARY_PATHS "${EXTRA_LD_LIBRARY_PATHS};${LIBMEMCACHED_LIB_DIR}" CACHE INTERNAL "" )
else()
  add_definitions( -DNOLIBMEMCACHED )
endif()

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
  legacy_accAPI.h
  legacy_client.h
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${fmt_SOURCE_DIR}/include
)

add_library( client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )
target_link_libraries( client-objects PUBLIC fmt::fmt )

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )
  target_link_libraries( fatclient-objects PUBLIC fmt::fmt )

  target_compile_definitions( fatclient-objects PUBLIC -DFATCLIENT )
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

set( CLIENT_LINK_LIBS ${OPENSSL_LIBRARIES} ${CACHE_LIBRARIES} ${LIBXML2_LIBRARIES} fmt::fmt )
if( NOT CLIENT_ONLY )
  if( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} Iconv::Iconv liblzma::liblzma ZLIB::ZLIB crypt32 )
  endif()
endif()

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ws2_32 ${XDR_LIBRARIES} )
  if ( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dlfcn-win32::dl crypt32 stdc++ pthread )
  else()
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ${TIRPC_LIBRARIES} )
endif()

target_link_libraries( client-static PUBLIC ${CLIENT_LINK_LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( client-shared PUBLIC ${CLIENT_LINK_LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient-static PUBLIC plugins-static ${CLIENT_LINK_LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient-shared PUBLIC plugins-shared ${CLIENT_LINK_LIBS} )
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
  if( WIN32 )
    set_target_properties( client-shared
      PROPERTIES
        COMPILE_FLAGS -DLIBRARY_EXPORTS
        IMPORT_SUFFIX ${IMPLIB_SUFFIX}
    )
  endif()

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
    if( WIN32 )
      set_target_properties( fatclient-shared
        PROPERTIES
          COMPILE_FLAGS -DLIBRARY_EXPORTS
          IMPORT_SUFFIX ${IMPLIB_SUFFIX}
      )
    endif()

    install( TARGETS fatclient-shared
      DESTINATION lib
  )
  endif()
endif()
