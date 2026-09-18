#include "oauth_authentication.h"
#include "claim_policy.h"
#include "jwks_cache.h"
#include "http_get.h"
#include "authLog.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
// nlohmann traits, not the jwt-cpp default picojson traits: UDA already parses
// every JSON document with nlohmann, and this defines JWT_DISABLE_PICOJSON so the
// second JSON library is not compiled at all.
#include <jwt-cpp/traits/nlohmann-json/defaults.h>

namespace uda {
namespace authentication {

namespace {

using json = nlohmann::json;

std::string fetch_jwks_cached(const std::string& uri,
                              const HttpFetcher& fetcher,
                              JwksCache& cache,
                              bool force_refresh = false)
{

    if (!force_refresh) {
        std::lock_guard<std::mutex> lock(cache.mutex);
        const auto it = cache.entries.find(uri);
        if (it != cache.entries.end()) {
            const auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - it->second.fetched_at).count();
            if (age < cache.ttl_seconds) {
                return it->second.json_str;
            }
        }
    }

    // Fetch outside the lock — network I/O must not hold the mutex
    const std::string result = fetcher(uri);

    {
        std::lock_guard<std::mutex> lock(cache.mutex);
        cache.entries[uri] = {result, std::chrono::steady_clock::now()};
    }

    return result;
}

} // namespace

// -------------------------------------------------------------------------
// curl_http_fetch — the OIDC discovery/JWKS policy over the shared http_get

std::string curl_http_fetch(const std::string& url)
{
    return http_get(url); // defaults are the OIDC discovery/JWKS policy
}

// -------------------------------------------------------------------------
// OidcCTX — resolves JWKS URI and verifies tokens

namespace detail {

class OidcCTX {
public:
    OidcCTX(const OidcConfig& cfg, const HttpFetcher& fetcher, JwksCache& cache)
        : cfg_(cfg), fetcher_(fetcher), cache_(cache)
    {
        if (!cfg_.jwks_uri.empty()) {
            jwks_uri_ = cfg_.jwks_uri;
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using configured JWKS URI %s\n", jwks_uri_.c_str());
            return;
        }

        if (cfg_.issuer.empty()) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "OIDC issuer is not configured; set UDA_SERVER_OIDC_ISSUER "
                "(or legacy UDA_SERVER_KEYCLOAK_REALM)");
        }

        const std::string discovery_url = cfg_.issuer + "/.well-known/openid-configuration";

        // Check discovery cache before fetching (jwks_uri is stable for the life of the process)
        {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            const auto it = cache_.discovery_uris.find(discovery_url);
            if (it != cache_.discovery_uris.end()) {
                jwks_uri_ = it->second;
                AUTH_LOG(UDA_LOG_DEBUG,
                    "Auth: using cached JWKS URI from discovery: %s\n", jwks_uri_.c_str());
                return;
            }
        }

        AUTH_LOG(UDA_LOG_DEBUG, "Auth: fetching OIDC discovery from %s\n", discovery_url.c_str());

        json discovery;
        try {
            discovery = json::parse(fetcher_(discovery_url));
        } catch (const AuthError&) {
            throw;
        } catch (const std::exception& e) {
            throw AuthError(AuthErrorCode::DiscoveryFailed,
                std::string("OIDC discovery failed: ") + e.what());
        }

        if (cfg_.verify_issuer && discovery.contains("issuer")) {
            const std::string disc_issuer = discovery["issuer"];
            if (disc_issuer != cfg_.issuer) {
                throw AuthError(AuthErrorCode::InvalidConfig,
                    "OIDC discovery issuer mismatch: configured '" + cfg_.issuer +
                    "' but discovery returned '" + disc_issuer + "'");
            }
        }

        jwks_uri_ = discovery.value("jwks_uri", "");
        if (jwks_uri_.empty()) {
            throw AuthError(AuthErrorCode::DiscoveryFailed,
                "OIDC discovery document missing jwks_uri field");
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: resolved JWKS URI: %s\n", jwks_uri_.c_str());

        {
            std::lock_guard<std::mutex> lock(cache_.mutex);
            cache_.discovery_uris[discovery_url] = jwks_uri_;
        }
    }

    // Verify token; returns decoded payload map.
    // On an unknown key id, refreshes the JWKS once and retries — this is what makes
    // issuer key rotation transparent. Never logs or exposes the raw token.
    [[nodiscard]] PayloadType verify_token(const std::string& token) const
    {
        // jwt::decode throws its own exception types for a token that is not even
        // structurally a JWT (bad base64, wrong segment count). Those are token faults,
        // not internal errors, so they are classified here rather than escaping the
        // AuthError family and surfacing to the client as a generic 999.
        const auto decoded = [&] {
            try {
                return jwt::decode(token);
            } catch (const std::exception& e) {
                throw AuthError(AuthErrorCode::InvalidToken,
                    std::string("Token is not a well-formed JWT: ") + e.what());
            }
        }();

        // Without a key id there is nothing to look up in the JWKS, and no amount of
        // refreshing will help. Fail here rather than letting the missing-claim exception
        // masquerade as a key-rotation signal below.
        if (!decoded.has_key_id()) {
            throw AuthError(AuthErrorCode::InvalidToken,
                "Token header has no 'kid'; cannot select a signing key from the issuer's JWKS");
        }
        const std::string kid = decoded.get_key_id();

        std::string jwks_json;
        try {
            jwks_json = fetch_jwks_cached(jwks_uri_, fetcher_, cache_);
        } catch (const AuthError&) {
            throw;
        } catch (const std::exception& e) {
            throw AuthError(AuthErrorCode::JwksFetchFailed,
                std::string("JWKS fetch failed: ") + e.what());
        }

        try {
            return verify_with_jwks(decoded, jwks_json);
        } catch (const jwt::error::claim_not_present_exception&) {
            // The key id is absent from the cached key set — the issuer has most likely
            // rotated its signing keys. Refresh once and retry.
            AUTH_LOG(UDA_LOG_DEBUG,
                "Auth: key id not found in cached JWKS, refreshing (possible key rotation)\n");
        }

        try {
            const std::string fresh = fetch_jwks_cached(jwks_uri_, fetcher_, cache_, true);
            return verify_with_jwks(decoded, fresh);
        } catch (const AuthError&) {
            throw;
        } catch (const jwt::error::claim_not_present_exception&) {
            throw AuthError(AuthErrorCode::InvalidToken,
                "Token key id '" + kid + "' is not present in the issuer's JWKS");
        } catch (const std::exception& e) {
            throw AuthError(AuthErrorCode::InvalidToken, e.what());
        }
    }

private:
    // Map a jwt-cpp verification failure onto a typed AuthErrorCode. The distinction
    // matters operationally: "your token expired" and "bad signature" call for very
    // different responses, and they end up in different refused_requests.log reason codes.
    [[nodiscard]] AuthErrorCode classify_verification_error(
        const std::error_code& ec,
        const jwt::decoded_jwt<jwt::traits::nlohmann_json>& decoded) const
    {
        using jwt::error::token_verification_error;

        if (ec == token_verification_error::token_expired) {
            return AuthErrorCode::TokenExpired;
        }
        if (ec == token_verification_error::audience_missmatch) {
            return AuthErrorCode::TokenBadAudience;
        }
        // A claim mismatch is reported generically, so check whether it was the issuer.
        if (ec == token_verification_error::claim_value_missmatch ||
            ec == token_verification_error::missing_claim) {
            if (cfg_.verify_issuer && !cfg_.issuer.empty()) {
                const bool issuer_ok = decoded.has_issuer() && decoded.get_issuer() == cfg_.issuer;
                if (!issuer_ok) {
                    return AuthErrorCode::TokenBadIssuer;
                }
            }
        }
        return AuthErrorCode::InvalidToken;
    }

    // Verify decoded token against a given JWKS JSON string.
    // Throws AuthError(InvalidConfig) for bad algorithm config and a classified
    // AuthError for claim/signature failures. Lets jwt-cpp's claim_not_present_exception
    // escape so that verify_token can retry against a refreshed key set.
    [[nodiscard]] PayloadType verify_with_jwks(
        const jwt::decoded_jwt<jwt::traits::nlohmann_json>& decoded,
        const std::string& jwks_json) const
    {
        // get_jwk throws claim_not_present_exception when the key id is absent; that is
        // the rotation signal verify_token retries on.
        const auto jwk = jwt::parse_jwks(jwks_json).get_jwk(decoded.get_key_id());
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWK key found (kid=%s)\n", decoded.get_key_id().c_str());

        std::string pub_key_pem;
        if (jwk.has_x5c()) {
            const auto x5c = jwk.get_x5c_key_value();
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using x5c component for signature verification\n");
            pub_key_pem = jwt::helper::convert_base64_der_to_pem(x5c);
        } else if (jwk.has_jwk_claim("n") && jwk.has_jwk_claim("e")) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using RSA n+e components for signature verification\n");
            const auto modulus  = jwk.get_jwk_claim("n").as_string();
            const auto exponent = jwk.get_jwk_claim("e").as_string();
            pub_key_pem = jwt::helper::create_public_key_from_rsa_components(modulus, exponent);
        } else if (jwk.has_jwk_claim("x") && jwk.has_jwk_claim("y")) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using EC x+y components for signature verification\n");
            const auto curve = jwk.has_jwk_claim("crv") ? jwk.get_jwk_claim("crv").as_string() : "";
            const auto x     = jwk.get_jwk_claim("x").as_string();
            const auto y     = jwk.get_jwk_claim("y").as_string();
            pub_key_pem = jwt::helper::create_public_key_from_ec_components(curve, x, y);
        } else {
            throw AuthError(AuthErrorCode::InvalidToken,
                "JWKS entry for kid '" + decoded.get_key_id() +
                "' has no usable key material (expected x5c, RSA n/e, or EC x/y)");
        }

        const auto leeway = static_cast<size_t>(
            cfg_.clock_skew_seconds > 0 ? cfg_.clock_skew_seconds : 0);
        auto verifier = jwt::verify().leeway(leeway);

        // jwt-cpp always validates exp and nbf when present — this is correct behaviour
        // and there is no supported way to disable it.

        bool any_alg = false;
        for (const auto& alg : cfg_.allowed_algs) {
            if      (alg == "RS256") { verifier.allow_algorithm(jwt::algorithm::rs256(pub_key_pem, "", "", "")); any_alg = true; }
            else if (alg == "RS384") { verifier.allow_algorithm(jwt::algorithm::rs384(pub_key_pem, "", "", "")); any_alg = true; }
            else if (alg == "RS512") { verifier.allow_algorithm(jwt::algorithm::rs512(pub_key_pem, "", "", "")); any_alg = true; }
            else if (alg == "ES256") { verifier.allow_algorithm(jwt::algorithm::es256(pub_key_pem, "", "", "")); any_alg = true; }
            else if (alg == "ES384") { verifier.allow_algorithm(jwt::algorithm::es384(pub_key_pem, "", "", "")); any_alg = true; }
            else if (alg == "ES512") { verifier.allow_algorithm(jwt::algorithm::es512(pub_key_pem, "", "", "")); any_alg = true; }
            else {
                AUTH_LOG(UDA_LOG_WARN,
                    "Auth: unsupported algorithm '%s' in UDA_SERVER_OIDC_ALLOWED_ALGS "
                    "(supported: RS256, RS384, RS512, ES256, ES384, ES512)\n", alg.c_str());
            }
        }
        if (!any_alg) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "No supported algorithms configured in UDA_SERVER_OIDC_ALLOWED_ALGS; "
                "supported: RS256, RS384, RS512, ES256, ES384, ES512");
        }

        if (cfg_.verify_issuer && !cfg_.issuer.empty()) {
            verifier.with_issuer(cfg_.issuer);
        }
        if (cfg_.verify_audience && !cfg_.audience.empty()) {
            verifier.with_audience(cfg_.audience);
        }

        // Use the error_code overload so the failure can be classified rather than
        // flattened into one opaque exception.
        std::error_code ec;
        verifier.verify(decoded, ec);
        if (ec) {
            throw AuthError(classify_verification_error(ec, decoded), ec.message());
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWT signature and claims verified\n");

        // String claims are stored as plain strings; arrays, objects and numbers are
        // JSON-serialised so ClaimPolicy can parse them back.
        const json payload_json = json::parse(decoded.get_payload());
        PayloadType payload_map;
        for (const auto& [key, val] : payload_json.items()) {
            if (val.is_string()) {
                payload_map.emplace(key, val.get<std::string>());
            } else {
                payload_map.emplace(key, val.dump());
            }
        }
        return payload_map;
    }

    const OidcConfig& cfg_;
    const HttpFetcher& fetcher_;
    JwksCache& cache_;
    std::string jwks_uri_;
};

} // namespace detail

// -------------------------------------------------------------------------
// authenticate_impl — shared implementation used by both public overloads

static PayloadType authenticate_impl(const std::string& token,
                                     const OidcConfig& cfg,
                                     const HttpFetcher& fetcher,
                                     JwksCache& cache)
{
    if (token.empty()) {
        throw AuthError(AuthErrorCode::MissingToken,
            "Authentication is enabled but no bearer token was provided");
    }

    if (cfg.issuer.empty() && cfg.jwks_uri.empty()) {
        throw AuthError(AuthErrorCode::InvalidConfig,
            "OIDC authentication is enabled but no issuer is configured; "
            "set UDA_SERVER_OIDC_ISSUER or UDA_SERVER_KEYCLOAK_REALM");
    }

    if (cfg.legacy_keycloak_config) {
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: using legacy Keycloak env var config; "
            "prefer UDA_SERVER_OIDC_ISSUER / UDA_SERVER_OIDC_CLIENT_ID\n");
    }

    // Require a meaningful claim policy: audience, legacy azp (Keycloak client_id), or
    // required_claims. Without at least one, any valid token from the issuer is accepted.
    // Set UDA_SERVER_OIDC_POLICY=none to explicitly opt out (logs a warning).
    const bool has_audience        = !cfg.audience.empty();
    const bool has_legacy_azp      = cfg.legacy_keycloak_config && !cfg.client_id.empty();
    const bool has_required_claims = !cfg.required_claims.empty();

    if (!has_audience && !has_legacy_azp && !has_required_claims) {
        const char* policy_env = std::getenv("UDA_SERVER_OIDC_POLICY");
        const bool none_policy = (policy_env != nullptr && std::string(policy_env) == "none");
        if (!none_policy) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "OIDC configuration has no claim policy (audience/required_claims/client_id). "
                "Set UDA_SERVER_OIDC_POLICY=none to explicitly allow any valid token.");
        }
        AUTH_LOG(UDA_LOG_WARN,
            "Auth: UDA_SERVER_OIDC_POLICY=none — any valid token from the issuer will be "
            "accepted without audience or claim checks. Not recommended for production.\n");
    }

    AUTH_LOG(UDA_LOG_DEBUG,
        "Auth: OIDC issuer=%s client_id=%s audience=%s verify_issuer=%d "
        "verify_audience=%d leeway=%ds\n",
        cfg.issuer.c_str(), cfg.client_id.c_str(), cfg.audience.c_str(),
        cfg.verify_issuer, cfg.verify_audience, cfg.clock_skew_seconds);

    const detail::OidcCTX ctx(cfg, fetcher, cache);
    PayloadType payload = ctx.verify_token(token);

    // Legacy Keycloak azp == client_id check when no explicit required_claims is set
    if (has_legacy_azp && cfg.required_claims.empty()) {
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: applying legacy claim policy: azp must equal client_id '%s'\n",
            cfg.client_id.c_str());
        const auto it = payload.find("azp");
        if (it == payload.end() || it->second != cfg.client_id) {
            throw AuthError(AuthErrorCode::ClaimPolicyFailed,
                "Token claim 'azp' does not match configured client_id (legacy policy)");
        }
    }

    if (!cfg.required_claims.empty()) {
        ClaimPolicy policy;
        try {
            policy = ClaimPolicy::parse(cfg.required_claims);
        } catch (const std::exception& e) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                std::string("Malformed UDA_SERVER_OIDC_REQUIRED_CLAIMS: ") + e.what());
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: applying required_claims policy\n");
        std::string policy_error;
        if (!policy.check(payload, policy_error)) {
            throw AuthError(AuthErrorCode::ClaimPolicyFailed,
                "Token claim policy check failed: " + policy_error);
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: required_claims policy passed\n");
    }

    AUTH_LOG(UDA_LOG_DEBUG, "Auth: token validation succeeded\n");
    return payload;
}

// -------------------------------------------------------------------------
// Public authenticate() overloads

PayloadType authenticate(const std::string& token,
                         const OidcConfig& cfg,
                         const HttpFetcher& fetcher)
{
    return authenticate_impl(token, cfg, fetcher, global_jwks_cache());
}

PayloadType authenticate(const std::string& token,
                         const OidcConfig& cfg,
                         const HttpFetcher& fetcher,
                         JwksCache& cache)
{
    return authenticate_impl(token, cfg, fetcher, cache);
}

PayloadType authenticate(const std::string& token)
{
    return authenticate_impl(token, OidcConfig::from_env(), curl_http_fetch, global_jwks_cache());
}

} // namespace authentication
} // namespace uda
