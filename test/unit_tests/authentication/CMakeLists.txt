if( ENABLE_TLS OR ENABLE_OIDC_AUTH )
  find_package( OpenSSL REQUIRED )
endif()
if( ENABLE_OIDC_AUTH )
  find_package( CURL REQUIRED )
endif()

if( ENABLE_OIDC_AUTH )

set( AUTH_TEST_LOGGING_OBJECTS $<TARGET_OBJECTS:logging-client-objects> )

# authentication-test-objects: compile authentication sources without SSLAUTHENTICATION
# (avoids pulling in OpenSSL client/server SSL code) so unit tests can link cleanly.
add_library( authentication-test-objects OBJECT
    ${CMAKE_SOURCE_DIR}/source/authentication/oauth_authentication.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/oidc_config.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/claim_policy.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/authLog.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/jwks_cache.cpp
)
target_include_directories( authentication-test-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( authentication-test-objects PRIVATE CURL::libcurl OpenSSL::Crypto )

# oidc_config tests
add_executable( test_oidc_config oidc_config.test.cpp ${AUTH_TEST_LOGGING_OBJECTS} )
target_include_directories( test_oidc_config PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_oidc_config PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
    CURL::libcurl
)
add_test( NAME TestOidcConfig COMMAND test_oidc_config )

# claim_policy tests
add_executable( test_claim_policy claim_policy.test.cpp ${AUTH_TEST_LOGGING_OBJECTS} )
target_include_directories( test_claim_policy PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_claim_policy PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
)
add_test( NAME TestClaimPolicy COMMAND test_claim_policy )

# oidc_verify tests (JWT sign/verify using mock HTTP fetcher)
add_executable( test_oidc_verify oidc_verify.test.cpp ${AUTH_TEST_LOGGING_OBJECTS} )
target_include_directories( test_oidc_verify PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_oidc_verify PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    CURL::libcurl
)
add_test( NAME TestOidcVerify COMMAND test_oidc_verify )

# provider_matrix tests (generic OIDC / Keycloak / SciTokens claim shapes)
add_executable( test_provider_matrix provider_matrix.test.cpp ${AUTH_TEST_LOGGING_OBJECTS} )
target_include_directories( test_provider_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries( test_provider_matrix PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    CURL::libcurl
)
add_test( NAME TestProviderMatrix COMMAND test_provider_matrix )

# handshake_auth unit tests (version check, auth_mode check, null-byte check)
add_library( handshake-auth-test-objects OBJECT
    ${CMAKE_SOURCE_DIR}/source/server/handshake_auth.cpp
)
target_include_directories( handshake-auth-test-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)

add_executable( test_server_auth server_auth.test.cpp ${AUTH_TEST_LOGGING_OBJECTS} )
target_include_directories( test_server_auth PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_server_auth PRIVATE
    handshake-auth-test-objects
    authentication-test-objects
    Catch2::Catch2WithMain
    CURL::libcurl
)
add_test( NAME TestServerAuth COMMAND test_server_auth )

endif() # ENABLE_OIDC_AUTH

# TLS integration tests (in-memory handshakes using OpenSSL BIO pairs)
if( ENABLE_TLS )
add_executable( test_tls_integration tls_integration.test.cpp )
target_include_directories( test_tls_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_tls_integration PRIVATE
    Catch2::Catch2WithMain
    OpenSSL::SSL
    OpenSSL::Crypto
)
add_test( NAME TestTlsIntegration COMMAND test_tls_integration )
endif() # ENABLE_TLS
