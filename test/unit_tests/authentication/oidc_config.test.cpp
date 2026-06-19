#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string>
#include <vector>

#include <authentication/oidc_config.h>

using namespace uda::authentication;

namespace {

// RAII env var setter
struct EnvGuard {
    std::string name;
    explicit EnvGuard(const char* n, const char* v) : name(n) { setenv(n, v, 1); }
    ~EnvGuard() { unsetenv(name.c_str()); }
};

// Unset several vars, restore on destruction
struct EnvCleaner {
    std::vector<std::string> names;
    explicit EnvCleaner(std::initializer_list<const char*> ns) {
        for (const char* n : ns) { names.emplace_back(n); unsetenv(n); }
    }
    ~EnvCleaner() { for (const auto& n : names) unsetenv(n.c_str()); }
};

} // namespace

TEST_CASE("OidcConfig::from_env returns secure defaults when no vars are set", "[oidc_config]")
{
    EnvCleaner clean{
        "UDA_SERVER_OIDC_ISSUER", "UDA_SERVER_OIDC_CLIENT_ID",
        "UDA_SERVER_OIDC_AUDIENCE", "UDA_SERVER_OIDC_JWKS_URI",
        "UDA_SERVER_OIDC_ALLOWED_ALGS", "UDA_SERVER_OIDC_REQUIRED_CLAIMS",
        "UDA_SERVER_OIDC_VERIFY_ISSUER", "UDA_SERVER_OIDC_VERIFY_AUDIENCE",
        "UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS",
        "UDA_SERVER_KEYCLOAK_REALM", "UDA_SERVER_KEYCLOAK_CLIENT_ID",
    };

    const OidcConfig cfg = OidcConfig::from_env();

    REQUIRE( cfg.issuer.empty() );
    REQUIRE( cfg.client_id.empty() );
    REQUIRE( cfg.audience.empty() );
    REQUIRE( cfg.jwks_uri.empty() );
    REQUIRE( cfg.required_claims.empty() );
    REQUIRE( cfg.allowed_algs == std::vector<std::string>{"RS256"} );
    REQUIRE( cfg.verify_issuer == true );
    REQUIRE( cfg.verify_audience == false );
    REQUIRE( cfg.clock_skew_seconds == 60 );
    REQUIRE( cfg.legacy_keycloak_config == false );
}

TEST_CASE("OidcConfig::from_env reads OIDC preferred vars", "[oidc_config]")
{
    EnvCleaner clean{
        "UDA_SERVER_OIDC_ISSUER", "UDA_SERVER_OIDC_CLIENT_ID",
        "UDA_SERVER_OIDC_AUDIENCE", "UDA_SERVER_OIDC_JWKS_URI",
        "UDA_SERVER_OIDC_ALLOWED_ALGS", "UDA_SERVER_OIDC_REQUIRED_CLAIMS",
        "UDA_SERVER_OIDC_VERIFY_ISSUER", "UDA_SERVER_OIDC_VERIFY_AUDIENCE",
        "UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS",
        "UDA_SERVER_KEYCLOAK_REALM", "UDA_SERVER_KEYCLOAK_CLIENT_ID",
    };

    EnvGuard issuer("UDA_SERVER_OIDC_ISSUER", "https://sso.example.com/realm/test");
    EnvGuard client("UDA_SERVER_OIDC_CLIENT_ID", "my-client");
    EnvGuard aud("UDA_SERVER_OIDC_AUDIENCE", "my-service");
    EnvGuard jwks("UDA_SERVER_OIDC_JWKS_URI", "https://sso.example.com/jwks");
    EnvGuard algs("UDA_SERVER_OIDC_ALLOWED_ALGS", "RS256,RS512");
    EnvGuard claims("UDA_SERVER_OIDC_REQUIRED_CLAIMS", "azp:equals:my-client");
    EnvGuard skew("UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS", "120");

    const OidcConfig cfg = OidcConfig::from_env();

    REQUIRE( cfg.issuer    == "https://sso.example.com/realm/test" );
    REQUIRE( cfg.client_id == "my-client" );
    REQUIRE( cfg.audience  == "my-service" );
    REQUIRE( cfg.jwks_uri  == "https://sso.example.com/jwks" );
    REQUIRE( cfg.required_claims == "azp:equals:my-client" );
    REQUIRE( cfg.clock_skew_seconds == 120 );
    REQUIRE( cfg.legacy_keycloak_config == false );

    const std::vector<std::string> expected_algs = {"RS256", "RS512"};
    REQUIRE( cfg.allowed_algs == expected_algs );
}

TEST_CASE("OidcConfig::from_env sets legacy_keycloak_config when Keycloak vars used", "[oidc_config]")
{
    EnvCleaner clean{
        "UDA_SERVER_OIDC_ISSUER", "UDA_SERVER_OIDC_CLIENT_ID",
        "UDA_SERVER_KEYCLOAK_REALM", "UDA_SERVER_KEYCLOAK_CLIENT_ID",
    };

    SECTION("Only Keycloak REALM set") {
        EnvGuard realm("UDA_SERVER_KEYCLOAK_REALM", "https://sso.example.com/realm/uda");
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.issuer == "https://sso.example.com/realm/uda" );
        REQUIRE( cfg.legacy_keycloak_config == true );
    }

    SECTION("Only Keycloak CLIENT_ID set") {
        EnvGuard cid("UDA_SERVER_KEYCLOAK_CLIENT_ID", "uda-client");
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.client_id == "uda-client" );
        REQUIRE( cfg.legacy_keycloak_config == true );
    }

    SECTION("OIDC_ISSUER takes precedence over Keycloak REALM") {
        EnvGuard realm("UDA_SERVER_KEYCLOAK_REALM", "https://keycloak.example.com/realm");
        EnvGuard oidc("UDA_SERVER_OIDC_ISSUER", "https://preferred.example.com");
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.issuer == "https://preferred.example.com" );
        REQUIRE( cfg.legacy_keycloak_config == false );
    }
}

TEST_CASE("OidcConfig::from_env verify_audience defaults to true when audience is set", "[oidc_config]")
{
    EnvCleaner clean{
        "UDA_SERVER_OIDC_AUDIENCE", "UDA_SERVER_OIDC_VERIFY_AUDIENCE",
    };

    SECTION("audience set, no verify_audience override") {
        EnvGuard aud("UDA_SERVER_OIDC_AUDIENCE", "my-service");
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.verify_audience == true );
    }

    SECTION("no audience, no verify_audience override") {
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.verify_audience == false );
    }

    SECTION("verify_audience=0 overrides default-true when audience set") {
        EnvGuard aud("UDA_SERVER_OIDC_AUDIENCE", "my-service");
        EnvGuard va("UDA_SERVER_OIDC_VERIFY_AUDIENCE", "0");
        const OidcConfig cfg = OidcConfig::from_env();
        REQUIRE( cfg.verify_audience == false );
    }
}

TEST_CASE("OidcConfig::from_env allowed_algs defaults to RS256", "[oidc_config]")
{
    EnvCleaner clean{"UDA_SERVER_OIDC_ALLOWED_ALGS"};

    const OidcConfig cfg = OidcConfig::from_env();
    REQUIRE( cfg.allowed_algs.size() == 1 );
    REQUIRE( cfg.allowed_algs[0] == "RS256" );
}

TEST_CASE("OidcConfig::from_env parses comma-separated allowed_algs", "[oidc_config]")
{
    EnvCleaner clean{"UDA_SERVER_OIDC_ALLOWED_ALGS"};
    EnvGuard algs("UDA_SERVER_OIDC_ALLOWED_ALGS", "RS256,RS384,RS512");

    const OidcConfig cfg = OidcConfig::from_env();
    REQUIRE( cfg.allowed_algs.size() == 3 );
    REQUIRE( cfg.allowed_algs[0] == "RS256" );
    REQUIRE( cfg.allowed_algs[1] == "RS384" );
    REQUIRE( cfg.allowed_algs[2] == "RS512" );
}

TEST_CASE("OidcConfig::from_env clock_skew_seconds ignores negative values", "[oidc_config]")
{
    EnvCleaner clean{"UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS"};
    EnvGuard skew("UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS", "-5");

    const OidcConfig cfg = OidcConfig::from_env();
    REQUIRE( cfg.clock_skew_seconds == 60 ); // falls back to default on negative
}
