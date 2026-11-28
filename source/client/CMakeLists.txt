########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( Boost REQUIRED )

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

# Which dependencies the installed CMake package and pkg-config files must find for consumers.
# Cached because they are read at the top level, after this directory is processed.
function( uda_package_needs NAME )
  if( ${ARGN} )
    set( UDA_PACKAGE_NEEDS_${NAME} ON CACHE INTERNAL "" )
  else()
    set( UDA_PACKAGE_NEEDS_${NAME} OFF CACHE INTERNAL "" )
  endif()
endfunction()

uda_package_needs( TIRPC TIRPC_FOUND )
uda_package_needs( XDR XDR_FOUND )
uda_package_needs( LIBMEMCACHED TARGET LibMemcached::LibMemcached )
uda_package_needs( DLFCN_WIN32 WIN32 OR MINGW )
uda_package_needs( ICONV MINGW AND NOT CLIENT_ONLY )

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

# Include paths come from the object targets rather than directory-wide include_directories().
add_library( client-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )
target_include_directories( client-objects PRIVATE ${CMAKE_SOURCE_DIR}/source ${Boost_INCLUDE_DIRS} )
target_link_libraries( client-objects PUBLIC fmt::fmt PRIVATE OpenSSL::SSL OpenSSL::Crypto LibXml2::LibXml2 )

if( NOT CLIENT_ONLY )
  add_library( fatclient-objects OBJECT ${SRC_FILES} )
  target_include_directories( fatclient-objects PRIVATE ${CMAKE_SOURCE_DIR}/source ${Boost_INCLUDE_DIRS} )
  target_link_libraries( fatclient-objects PUBLIC fmt::fmt PRIVATE OpenSSL::SSL OpenSSL::Crypto LibXml2::LibXml2 )

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
add_library( UDA::client_static ALIAS client-static )
target_compile_definitions( client-static INTERFACE UDA_STATIC )
if( BUILD_SHARED_LIBS )
  add_library( client-shared SHARED ${CLIENT_OBJS} )
  add_library( UDA::client_shared ALIAS client-shared )
endif()

set( CLIENT_LINK_LIBS OpenSSL::SSL OpenSSL::Crypto fmt::fmt Threads::Threads )
# udaStructs.h includes parseXML.h, which exposes libxml2 types.
set( CLIENT_PUBLIC_LINK_LIBS LibXml2::LibXml2 )
if( TARGET LibMemcached::LibMemcached )
  list( APPEND CLIENT_LINK_LIBS LibMemcached::LibMemcached )
endif()
if( NOT CLIENT_ONLY )
  if( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} Iconv::Iconv liblzma::liblzma ZLIB::ZLIB crypt32 )
  endif()
endif()

if( MINGW OR WIN32 )
  target_link_libraries( client-objects PRIVATE XDR::XDR )
  if( TARGET fatclient-objects )
    target_link_libraries( fatclient-objects PRIVATE XDR::XDR )
  endif()
  list( APPEND CLIENT_LINK_LIBS ws2_32 )
  list( APPEND CLIENT_PUBLIC_LINK_LIBS XDR::XDR )
  if ( MINGW )
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dlfcn-win32::dl crypt32 stdc++ pthread )
  else()
    set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} dlfcn-win32::dl )
  endif()
elseif( TIRPC_FOUND )
  target_link_libraries( client-objects PRIVATE TIRPC::TIRPC )
  if( TARGET fatclient-objects )
    target_link_libraries( fatclient-objects PRIVATE TIRPC::TIRPC )
  endif()
  list( APPEND CLIENT_PUBLIC_LINK_LIBS TIRPC::TIRPC )
endif()

target_include_directories( client-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/source>
  $<INSTALL_INTERFACE:${UDA_INSTALL_INCLUDEDIR}>
)
set_target_properties( client-static PROPERTIES EXPORT_NAME client_static )

target_link_libraries( client-static
  PUBLIC ${CLIENT_PUBLIC_LINK_LIBS}
  PRIVATE ${CLIENT_LINK_LIBS}
)
if( BUILD_SHARED_LIBS )
  target_include_directories( client-shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/source>
    $<INSTALL_INTERFACE:${UDA_INSTALL_INCLUDEDIR}>
  )
  set_target_properties( client-shared PROPERTIES EXPORT_NAME client_shared )
  target_link_libraries( client-shared
    PUBLIC ${CLIENT_PUBLIC_LINK_LIBS}
    PRIVATE ${CLIENT_LINK_LIBS}
  )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient-static PUBLIC plugins-static ${CLIENT_LINK_LIBS} ${CLIENT_PUBLIC_LINK_LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient-shared PUBLIC plugins-shared ${CLIENT_LINK_LIBS} ${CLIENT_PUBLIC_LINK_LIBS} )
  endif()
endif()

uda_install_library( client-static ${PROJECT_NAME}_client EXPORT UDAClientTargets )
if( BUILD_SHARED_LIBS )
  uda_install_library( client-shared ${PROJECT_NAME}_client EXPORT UDAClientSharedTargets )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/client
  COMPONENT development
)

if( NOT CLIENT_ONLY )
  uda_install_library( fatclient-static fat${PROJECT_NAME}_client )
  if( BUILD_SHARED_LIBS )
    uda_install_library( fatclient-shared fat${PROJECT_NAME}_client )
  endif()
endif()
