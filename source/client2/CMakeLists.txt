########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( fmt REQUIRED )
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

########################################################################################################################
# Sources

set( SRC_FILES
  udaGetAPI.cpp
  udaPutAPI.cpp
  accAPI.cpp
  client.cpp
  thread_client.cpp
  client_environment.cpp
  make_request_block.cpp
  client_xdr_stream.cpp
  connection.cpp
  closedown.cpp
  host_list.cpp
  generate_errors.cpp
  handle.cpp
  exceptions.cpp
  updateSelectParms.cpp
)

set( HEADER_FILES
  udaGetAPI.h
  udaPutAPI.h
  accAPI.h
  error_codes.h
  client.hpp
  thread_client.hpp
  client_environment.hpp
  make_request_block.hpp
  client_xdr_stream.hpp
  connection.hpp
  closedown.hpp
  host_list.hpp
  generate_errors.hpp
  handle.hpp
  exceptions.hpp
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${fmt_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

add_library( client2-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )
target_link_libraries( client2-objects PRIVATE fmt::fmt )

if( NOT CLIENT_ONLY )
  add_library( fatclient2-objects OBJECT ${SRC_FILES} )
  target_link_libraries( fatclient2-objects PRIVATE fmt::fmt )

  target_compile_definitions( fatclient2-objects PRIVATE -DFATCLIENT )
endif()

set( CLIENT_OBJS
  $<TARGET_OBJECTS:client2-objects>
  $<TARGET_OBJECTS:clientserver-client-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-client-objects>
  $<TARGET_OBJECTS:structures-client-objects>
  $<TARGET_OBJECTS:authentication-client-objects>
)

set( FATCLIENT_OBJS
  $<TARGET_OBJECTS:fatclient2-objects>
  $<TARGET_OBJECTS:fatserver-objects>
  $<TARGET_OBJECTS:fatclientserver-objects>
  $<TARGET_OBJECTS:structures-server-objects>
  $<TARGET_OBJECTS:cache-objects>
  $<TARGET_OBJECTS:logging-server-objects>
)

add_library( client2-static STATIC ${CLIENT_OBJS} )
if( BUILD_SHARED_LIBS )
  add_library( client2-shared SHARED ${CLIENT_OBJS} )
endif()

set( CLIENT_LINK_LIBS ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES} fmt::fmt )
if( TARGET LibMemcached::LibMemcached )
  list( APPEND CLIENT_LINK_LIBS LibMemcached::LibMemcached )
endif()
if( NOT CLIENT_ONLY )
  set( CLIENT_LINK_LIBS ${CLIENT_LINK_LIBS} ${LIBXML2_LIBRARIES} )
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

target_link_libraries( client2-static PRIVATE ${CLIENT_LINK_LIBS} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( client2-shared PRIVATE ${CLIENT_LINK_LIBS} )
endif()

if( NOT CLIENT_ONLY )
  add_library( fatclient2-static STATIC ${FATCLIENT_OBJS} )
  target_link_libraries( fatclient2-static PRIVATE plugins-static ${CLIENT_LINK_LIBS} )
  
  if( BUILD_SHARED_LIBS )
    add_library( fatclient2-shared SHARED ${FATCLIENT_OBJS} )
    target_link_libraries( fatclient2-shared PRIVATE plugins-shared ${CLIENT_LINK_LIBS} )
  endif()
endif()

uda_install_library( client2-static ${PROJECT_NAME}_client2 )
if( BUILD_SHARED_LIBS )
  uda_install_library( client2-shared ${PROJECT_NAME}_client2 )
endif()

file( GLOB HEADER_FILES "*.h" )

install( FILES ${HEADER_FILES}
  DESTINATION ${UDA_INSTALL_INCLUDEDIR}/client2
  COMPONENT development
)

if( NOT CLIENT_ONLY )
  uda_install_library( fatclient2-static fat${PROJECT_NAME}_client2 )
  if( BUILD_SHARED_LIBS )
    uda_install_library( fatclient2-shared fat${PROJECT_NAME}_client2 )
  endif()
endif()
