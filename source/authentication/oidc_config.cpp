#include "oidc_config.h"

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace uda {
namespace authentication {

namespace {

static bool parse_bool_env(const char* var, bool default_val)
{
    const char* v = getenv(var);
    if (!v) return default_val;
    const std::string s(v);
    return s == "1" || s == "true" || s == "yes" || s == "on";
}

static std::vector<std::string> split_csv(const std::string& s)
{
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) result.push_back(token);
    }
    return result;
}

} // namespace

OidcConfig OidcConfig::from_env()
{
    OidcConfig cfg;

    // Issuer: prefer new var, fall back to Keycloak realm alias
    if (const char* v = getenv("UDA_SERVER_OIDC_ISSUER")) {
        cfg.issuer = v;
    } else if (const char* v = getenv("UDA_SERVER_KEYCLOAK_REALM")) {
        cfg.issuer = v;
        cfg.legacy_keycloak_config = true;
    }

    // Client ID: prefer new var, fall back to Keycloak client ID alias
    if (const char* v = getenv("UDA_SERVER_OIDC_CLIENT_ID")) {
        cfg.client_id = v;
    } else if (const char* v = getenv("UDA_SERVER_KEYCLOAK_CLIENT_ID")) {
        cfg.client_id = v;
        cfg.legacy_keycloak_config = true;
    }

    if (const char* v = getenv("UDA_SERVER_OIDC_AUDIENCE")) {
        cfg.audience = v;
    }

    if (const char* v = getenv("UDA_SERVER_OIDC_JWKS_URI")) {
        cfg.jwks_uri = v;
    }

    if (const char* v = getenv("UDA_SERVER_OIDC_ALLOWED_ALGS")) {
        cfg.allowed_algs = split_csv(v);
    }
    if (cfg.allowed_algs.empty()) {
        cfg.allowed_algs = {"RS256"};
    }

    cfg.verify_issuer   = parse_bool_env("UDA_SERVER_OIDC_VERIFY_ISSUER", true);
    cfg.verify_audience = parse_bool_env("UDA_SERVER_OIDC_VERIFY_AUDIENCE", !cfg.audience.empty());
    cfg.verify_expiry   = parse_bool_env("UDA_SERVER_OIDC_VERIFY_EXPIRY", true);
    cfg.verify_nbf      = parse_bool_env("UDA_SERVER_OIDC_VERIFY_NBF", true);
    cfg.verify_iat      = parse_bool_env("UDA_SERVER_OIDC_VERIFY_IAT", false);

    if (const char* v = getenv("UDA_SERVER_OIDC_CLOCK_SKEW_SECONDS")) {
        int skew = atoi(v);
        cfg.clock_skew_seconds = (skew >= 0) ? skew : 60;
    }

    if (const char* v = getenv("UDA_SERVER_OIDC_REQUIRED_CLAIMS")) {
        cfg.required_claims = v;
    }

    return cfg;
}

} // namespace authentication
} // namespace uda
