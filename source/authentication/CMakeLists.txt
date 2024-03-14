find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET)
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

# Sources

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
)

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

add_library( authentication-client-objects OBJECT udaClientSSL.cpp udaClientSSL.h udaServerSSL.h utils.cpp )
target_link_libraries( authentication-client-objects PRIVATE OpenSSL::SSL LibXml2::LibXml2 )
target_compile_definitions( authentication-client-objects PRIVATE -DOPENSSL_NO_DEPRECATED )

if( NOT CLIENT_ONLY )
  add_library( authentication-server-objects OBJECT udaServerSSL.cpp udaClientSSL.h utils.cpp )
  target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD -DOPENSSL_NO_DEPRECATED )
  target_link_libraries( authentication-server-objects PRIVATE OpenSSL::SSL LibXml2::LibXml2 )
endif()