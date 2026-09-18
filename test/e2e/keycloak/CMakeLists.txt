# The live-IdP token checker. Built only when OIDC is compiled in.
#
# It reuses authentication-test-objects (defined by test/unit_tests/authentication) rather
# than the full server object library: the same authentication sources, without the server
# transport plumbing this tool has no use for. The verification path it exercises —
# discovery, JWKS, signature, issuer, audience, claim policy — is the production one.
if( NOT ENABLE_OIDC_AUTH )
  return()
endif()

find_package( OpenSSL REQUIRED )
find_package( CURL REQUIRED )

add_executable( uda_token_check uda_token_check.cpp $<TARGET_OBJECTS:logging-client-objects> )
target_include_directories( uda_token_check PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( uda_token_check PRIVATE
  authentication-test-objects
  OpenSSL::Crypto
  CURL::libcurl
)
