#pragma once

#include <string>
#include <vector>

namespace uda {
namespace authentication {

// OIDC/JWT validation configuration, read from environment at authenticate() time.
//
// Preferred environment variables:
//   UDA_SERVER_OIDC_ISSUER           OIDC issuer URL
//   UDA_SERVER_OIDC_CLIENT_ID        OIDC client ID
//   UDA_SERVER_OIDC_AUDIENCE         Expected audience claim (optional)
//   UDA_SERVER_OIDC_JWKS_URI         Direct JWKS endpoint (skips discovery if set)
//   UDA_SERVER_OIDC_ALLOWED_ALGS     Comma-separated algorithm whitelist (default: RS256)
//   UDA_SERVER_OIDC_REQUIRED_CLAIMS  Semicolon-separated claim policy rules (see claim_policy.h)
//   UDA_SERVER_OIDC_VERIFY_ISSUER=1|0       (default: 1)
//   UDA_SERVER_OIDC_VERIFY_AUDIENCE=1|0     (default: 1 if audience set, else 0)
//   UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS=<n>  (default: 60)
//
//   UDA_SERVER_OIDC_POLICY=none  — accept any valid token (no audience/claims policy);
//                                  only for development/testing; logs a warning.
//
//   UDA_SERVER_OIDC_ALLOW_HTTP=1 — permit non-HTTPS JWKS/discovery URLs (testing only).
//
// Legacy aliases (still accepted):
//   UDA_SERVER_KEYCLOAK_REALM        -> UDA_SERVER_OIDC_ISSUER
//   UDA_SERVER_KEYCLOAK_CLIENT_ID    -> UDA_SERVER_OIDC_CLIENT_ID
//
// UDA_SERVER_AUTHENTICATION accepts: "OIDC" or "OAUTH" (both trigger OIDC/JWT validation).
//
// Note: jwt-cpp always validates exp and nbf when present in the token; there is no
// supported mechanism to disable these checks. Clock skew tolerance is configurable.

struct OidcConfig {
    std::string issuer;
    std::string client_id;
    std::string audience;
    std::string jwks_uri;
    std::vector<std::string> allowed_algs = {"RS256"};

    bool verify_issuer   = true;
    bool verify_audience = false; // set to true automatically if audience is non-empty
    int  clock_skew_seconds = 60;

    std::string required_claims;

    // True when issuer/client_id came from legacy UDA_SERVER_KEYCLOAK_* vars.
    bool legacy_keycloak_config = false;

    static OidcConfig from_env();
};

} // namespace authentication
} // namespace uda
