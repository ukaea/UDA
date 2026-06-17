// Unit tests for check_oidc_client_auth() in source/server/handshake_auth.cpp.
// These tests cover the path-gating logic (version check, auth_mode check, auth_type
// check, null-byte check).  They do NOT reach JWT validation — no RSA keys needed.

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <vector>

#include <authentication/oauth_authentication.h>
#include <clientserver/udaDefines.h>
#include <clientserver/udaErrors.h>
#include <clientserver/udaStructs.h>
#include <server/handshake_auth.h>

using namespace uda::server;
using uda::authentication::HttpFetcher;

namespace {

// Null fetcher — should never be called by the tests in this file since they all
// fail before reaching the token-validation step.
HttpFetcher never_fetch()
{
    return [](const std::string& url) -> std::string {
        throw std::runtime_error("HTTP fetcher called unexpectedly in server_auth test: " + url);
    };
}

// Build a minimal CLIENT_BLOCK with sensible defaults for protocol-11 clients.
CLIENT_BLOCK make_client_block(int version = 11)
{
    CLIENT_BLOCK cb{};
    cb.version = version;
    cb.timeout = 60;
    return cb;
}

} // namespace

// ---------------------------------------------------------------------------
// auth_mode validation

TEST_CASE("check_oidc_client_auth returns InvalidConfig for unknown auth_mode", "[server_auth]")
{
    CLIENT_BLOCK cb = make_client_block(11);
    const auto result = check_oidc_client_auth(&cb, "SAML", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_INVALID_CONFIG );
    // Message should mention the bad value
    REQUIRE( result.message.find("SAML") != std::string::npos );
}

TEST_CASE("check_oidc_client_auth accepts OIDC and OAUTH as auth_mode spellings", "[server_auth]")
{
    // Both spellings should not fail on the mode check. They WILL fail later (no token),
    // but the error code changes to MissingToken, not InvalidConfig.
    for (const std::string& mode : {"OIDC", "OAUTH"}) {
        CLIENT_BLOCK cb = make_client_block(11);
        cb.authenticationBlock.authentication_type = 0; // not OAUTH — triggers MissingToken
        const auto result = check_oidc_client_auth(&cb, mode, never_fetch());
        REQUIRE( result.failed );
        REQUIRE( result.error_code == UDA_AUTH_ERR_MISSING_TOKEN );
    }
}

// ---------------------------------------------------------------------------
// Old-client version check

TEST_CASE("check_oidc_client_auth returns MissingToken for old protocol client", "[server_auth]")
{
    CLIENT_BLOCK cb = make_client_block(7);
    const auto result = check_oidc_client_auth(&cb, "OIDC", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_MISSING_TOKEN );
    // Message must contain both "protocol" and "upgrade" to be actionable
    REQUIRE( result.message.find("protocol") != std::string::npos );
    REQUIRE( result.message.find("upgrade") != std::string::npos );
}

TEST_CASE("check_oidc_client_auth version 10 client also fails with MissingToken", "[server_auth]")
{
    CLIENT_BLOCK cb = make_client_block(10);
    const auto result = check_oidc_client_auth(&cb, "OIDC", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_MISSING_TOKEN );
}

// ---------------------------------------------------------------------------
// Missing bearer token (version-11 client, wrong authentication_type)

TEST_CASE("check_oidc_client_auth returns MissingToken when authentication_type is not OAUTH", "[server_auth]")
{
    CLIENT_BLOCK cb = make_client_block(11);
    cb.authenticationBlock.authentication_type = 0; // not UDA_AUTHENTICATION_OAUTH
    const auto result = check_oidc_client_auth(&cb, "OIDC", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_MISSING_TOKEN );
}

// ---------------------------------------------------------------------------
// Null byte in payload

TEST_CASE("check_oidc_client_auth returns InvalidToken when bearer token contains null byte", "[server_auth]")
{
    // Payload with an embedded null: "abc\0def"
    std::vector<unsigned char> payload = {'a', 'b', 'c', '\0', 'd', 'e', 'f'};

    CLIENT_BLOCK cb = make_client_block(11);
    cb.authenticationBlock.authentication_type = UDA_AUTHENTICATION_OAUTH;
    cb.authenticationBlock.payload_length = static_cast<unsigned int>(payload.size());
    cb.authenticationBlock.payload = payload.data();

    const auto result = check_oidc_client_auth(&cb, "OIDC", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_INVALID_TOKEN );
}

TEST_CASE("check_oidc_client_auth null byte at start of payload is rejected", "[server_auth]")
{
    std::vector<unsigned char> payload = {'\0', 'e', 'y', 'J'};

    CLIENT_BLOCK cb = make_client_block(11);
    cb.authenticationBlock.authentication_type = UDA_AUTHENTICATION_OAUTH;
    cb.authenticationBlock.payload_length = static_cast<unsigned int>(payload.size());
    cb.authenticationBlock.payload = payload.data();

    const auto result = check_oidc_client_auth(&cb, "OIDC", never_fetch());
    REQUIRE( result.failed );
    REQUIRE( result.error_code == UDA_AUTH_ERR_INVALID_TOKEN );
}

// ---------------------------------------------------------------------------
// Successful path through gating (reaches token validation, fails on bad config)

TEST_CASE("check_oidc_client_auth passes gating and fails at token validation with no env config", "[server_auth]")
{
    // This test exercises the gating checks passing (v11 client, OIDC mode, OAUTH type, no null bytes).
    // With no UDA_SERVER_OIDC_* env vars set it will reach authenticate() and fail with
    // InvalidConfig — which is correct behaviour and confirms the gating didn't reject early.
    const std::string fake_token = "eyJhbGciOiJSUzI1NiJ9.e30.fake";

    std::vector<unsigned char> payload(fake_token.begin(), fake_token.end());

    CLIENT_BLOCK cb = make_client_block(11);
    cb.authenticationBlock.authentication_type = UDA_AUTHENTICATION_OAUTH;
    cb.authenticationBlock.payload_length = static_cast<unsigned int>(payload.size());
    cb.authenticationBlock.payload = payload.data();

    const auto result = check_oidc_client_auth(&cb, "OIDC", [](const std::string&) { return "{}"; });
    REQUIRE( result.failed );
    // The error should be config or token-related, not a gating error
    REQUIRE( result.error_code != 999 );
    REQUIRE( result.error_code >= 700 );
    REQUIRE( result.error_code <= 712 );
}
