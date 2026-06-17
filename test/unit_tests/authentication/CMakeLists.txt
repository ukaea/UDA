find_package( OpenSSL REQUIRED )

# authentication-test-objects: compile authentication sources without SSLAUTHENTICATION
# (avoids pulling in OpenSSL client/server SSL code) so unit tests can link cleanly.
add_library( authentication-test-objects OBJECT
    ${CMAKE_SOURCE_DIR}/source/authentication/oauth_authentication.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/oidc_config.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/claim_policy.cpp
    ${CMAKE_SOURCE_DIR}/source/authentication/authLog.cpp
)
target_include_directories( authentication-test-objects PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( authentication-test-objects PRIVATE curl )

# oidc_config tests
add_executable( test_oidc_config oidc_config.test.cpp )
target_include_directories( test_oidc_config PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_oidc_config PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
    curl
)
add_test( NAME TestOidcConfig COMMAND test_oidc_config )

# claim_policy tests
add_executable( test_claim_policy claim_policy.test.cpp )
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
add_executable( test_oidc_verify oidc_verify.test.cpp )
target_include_directories( test_oidc_verify PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/extlib
)
target_link_libraries( test_oidc_verify PRIVATE
    authentication-test-objects
    Catch2::Catch2WithMain
    OpenSSL::Crypto
    curl
)
add_test( NAME TestOidcVerify COMMAND test_oidc_verify )

# TLS integration tests (in-memory handshakes using OpenSSL BIO pairs)
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
