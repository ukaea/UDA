if( NOT ENABLE_TLS AND NOT ENABLE_OIDC_AUTH )
  # Neither TLS nor OIDC is enabled; define empty stub targets so that
  # source/client and source/server always have these target names available.
  add_library( authentication-client-objects OBJECT )
  if( NOT CLIENT_ONLY )
    add_library( authentication-server-objects OBJECT )
  endif()
  return()
endif()

if( ENABLE_TLS OR ENABLE_OIDC_AUTH )
  find_package( OpenSSL REQUIRED )
endif()

if( ENABLE_OIDC_AUTH )
  find_package( CURL REQUIRED )
endif()

find_package( LibXml2 REQUIRED )

if( WIN32 OR MINGW )
  find_package( XDR REQUIRED )
else()
  find_package( TIRPC QUIET )
  if( TIRPC_FOUND )
    add_definitions( -D__TIRPC__ )
  endif()
endif()

set( HEADER_FILES
  oauth_authentication.h authLog.h oidc_config.h claim_policy.h
  jwks_cache.h tls_policy.h refusal_log.h
)

include_directories(
  ${CMAKE_SOURCE_DIR}/source
  ${LIBXML2_INCLUDE_DIR}
)
if( ENABLE_TLS OR ENABLE_OIDC_AUTH )
  include_directories( ${OPENSSL_INCLUDE_DIR} )
endif()

if( MINGW OR WIN32 )
  include_directories( ${XDR_INCLUDE_DIR} )
elseif( TIRPC_FOUND )
  include_directories( ${TIRPC_INCLUDE_DIR} )
endif()

# Client objects: TLS only (clients send tokens but do not validate them)
add_library( authentication-client-objects OBJECT udaClientSSL.cpp authLog.cpp utils.cpp )
target_include_directories( authentication-client-objects PRIVATE ${CMAKE_SOURCE_DIR}/extlib )
if( ENABLE_TLS )
  target_link_libraries( authentication-client-objects PRIVATE OpenSSL::SSL LibXml2::LibXml2 )
endif()

if( NOT CLIENT_ONLY )
  # Server objects: TLS + OIDC — include only what is enabled
  set( SERVER_AUTH_SRCS authLog.cpp utils.cpp refusal_log.cpp )
  if( ENABLE_TLS )
    list( APPEND SERVER_AUTH_SRCS udaServerSSL.cpp )
  endif()
  if( ENABLE_OIDC_AUTH )
    list( APPEND SERVER_AUTH_SRCS
      oauth_authentication.cpp oidc_config.cpp claim_policy.cpp jwks_cache.cpp )
  endif()

  add_library( authentication-server-objects OBJECT ${SERVER_AUTH_SRCS} )
  target_include_directories( authentication-server-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/extlib ${CMAKE_SOURCE_DIR}/source )
  target_compile_definitions( authentication-server-objects PRIVATE -DSERVERBUILD )
  if( ENABLE_TLS )
    target_link_libraries( authentication-server-objects PRIVATE OpenSSL::SSL LibXml2::LibXml2 )
  endif()
  if( ENABLE_OIDC_AUTH )
    target_link_libraries( authentication-server-objects PRIVATE OpenSSL::Crypto CURL::libcurl )
  endif()
endif()

install( FILES ${HEADER_FILES}
  DESTINATION include/uda/authentication
)
