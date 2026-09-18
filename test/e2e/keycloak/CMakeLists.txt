# The live-IdP token checker. Built only when OIDC is compiled in.
#
# It reuses authentication-test-objects (defined by test/unit_tests/authentication) rather
# than the full server object library: the same authentication sources, without the server
# transport plumbing this tool has no use for. The verification path it exercises —
# discovery, JWKS, signature, issuer, audience, claim policy — is the production one.
if( NOT ENABLE_OIDC_AUTH )
  return()
endif()

# uda_connect_check drives a real connection end to end. It needs the client library, so
# it is only built when one is being produced.
if( TARGET client-static )
  add_executable( uda_connect_check uda_connect_check.cpp )
  target_include_directories( uda_connect_check PRIVATE ${CMAKE_SOURCE_DIR}/source )
  target_link_libraries( uda_connect_check PRIVATE client-static )
endif()

find_package( OpenSSL REQUIRED )
find_package( CURL REQUIRED )

add_executable( uda_token_check uda_token_check.cpp $<TARGET_OBJECTS:logging-client-objects> )
target_include_directories( uda_token_check PRIVATE ${CMAKE_SOURCE_DIR}/source )
target_link_libraries( uda_token_check PRIVATE
  authentication-test-objects
  nlohmann_json::nlohmann_json
  jwt-cpp::jwt-cpp
  OpenSSL::Crypto
  CURL::libcurl
)
