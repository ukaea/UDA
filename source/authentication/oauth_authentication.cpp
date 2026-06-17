#include "oauth_authentication.h"
#include "claim_policy.h"
#include "authLog.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

namespace uda {
namespace authentication {

// -------------------------------------------------------------------------
// cURL global init — once per process, never cleaned up (safe for a library)

namespace {

void ensure_curl_initialized()
{
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

size_t write_callback(void* contents, size_t size, size_t count, std::string* output)
{
    const size_t total = size * count;
    output->append(static_cast<char*>(contents), total);
    return total;
}

using json = nlohmann::json;

// -------------------------------------------------------------------------
// JWKS cache — process-global, TTL-based, mutex-protected
//
// TTL is read once from UDA_SERVER_OIDC_JWKS_CACHE_TTL at first use.
// Fetches are done outside the lock to avoid holding it during network I/O.
// A force-refresh bypasses the TTL check (used on kid-not-found retry).

struct JwksEntry {
    std::string json_str;
    std::chrono::steady_clock::time_point fetched_at;
};

struct JwksCache {
    std::mutex mutex;
    std::unordered_map<std::string, JwksEntry> entries;
    // Caches discovery_url -> resolved jwks_uri. The jwks_uri from a discovery document is
    // stable across the lifetime of a process; no TTL is needed here.
    std::unordered_map<std::string, std::string> discovery_uris;
    int ttl_seconds;

    JwksCache() {
        const char* ttl_env = std::getenv("UDA_SERVER_OIDC_JWKS_CACHE_TTL");
        int ttl = ttl_env ? std::atoi(ttl_env) : 300;
        ttl_seconds = ttl > 0 ? ttl : 300;
    }
};

JwksCache& global_jwks_cache()
{
    static JwksCache cache;
    return cache;
}

std::string fetch_jwks_cached(const std::string& uri,
                              const HttpFetcher& fetcher,
                              bool force_refresh = false)
{
    auto& cache = global_jwks_cache();

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
// curl_http_fetch — default HTTP fetcher with hardened options

std::string curl_http_fetch(const std::string& url)
{
    const bool allow_http = (std::getenv("UDA_SERVER_OIDC_ALLOW_HTTP") != nullptr);
    if (!allow_http) {
        const bool is_https = url.size() >= 8 && url.substr(0, 8) == "https://";
        if (!is_https) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "OIDC fetch rejected non-HTTPS URL '" + url + "' — "
                "set UDA_SERVER_OIDC_ALLOW_HTTP=1 to override (testing only)");
        }
    }

    ensure_curl_initialized();
    CURL* handle = curl_easy_init();
    if (!handle) {
        throw AuthError(AuthErrorCode::JwksFetchFailed, "curl_easy_init() failed");
    }

    std::string response;
    char errbuf[CURL_ERROR_SIZE] = {};

    curl_easy_setopt(handle, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,  write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER,    errbuf);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR,    1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS,      3L);

    const CURLcode rc = curl_easy_perform(handle);

    if (rc != CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(handle);
        throw AuthError(AuthErrorCode::JwksFetchFailed,
            "HTTP GET '" + url + "' failed: " +
            (errbuf[0] ? errbuf : curl_easy_strerror(rc)) +
            (http_code > 0 ? " (HTTP " + std::to_string(http_code) + ")" : ""));
    }

    curl_easy_cleanup(handle);
    return response;
}

// -------------------------------------------------------------------------
// OidcCTX — resolves JWKS URI and verifies tokens

namespace detail {

class OidcCTX {
public:
    OidcCTX(const OidcConfig& cfg, const HttpFetcher& fetcher)
        : cfg_(cfg), fetcher_(fetcher)
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
            auto& cache = global_jwks_cache();
            std::lock_guard<std::mutex> lock(cache.mutex);
            const auto it = cache.discovery_uris.find(discovery_url);
            if (it != cache.discovery_uris.end()) {
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
            auto& cache = global_jwks_cache();
            std::lock_guard<std::mutex> lock(cache.mutex);
            cache.discovery_uris[discovery_url] = jwks_uri_;
        }
    }

    // Verify token; returns decoded payload map.
    // On kid-not-found with a stale JWKS, refreshes once (handles key rotation).
    // Never logs or exposes the raw token.
    [[nodiscard]] PayloadType verify_token(const std::string& token) const
    {
        const auto decoded = jwt::decode(token);

        std::string jwks_json;
        try {
            jwks_json = fetch_jwks_cached(jwks_uri_, fetcher_);
        } catch (const AuthError&) {
            throw;
        } catch (const std::exception& e) {
            throw AuthError(AuthErrorCode::JwksFetchFailed,
                std::string("JWKS fetch failed: ") + e.what());
        }

        try {
            return verify_with_jwks(decoded, jwks_json);
        } catch (const AuthError&) {
            throw; // typed failures from verify_with_jwks — no retry
        } catch (const std::exception& e) {
            const std::string first_error = e.what();
            // Only retry on kid-not-found; all other jwt-cpp errors are not stale-cache issues.
            // The substring "kid not found" is load-bearing: it comes from jwt-cpp's
            // jwt_object_set::get_jwk() when the requested kid is absent from the key set.
            // If jwt-cpp changes this message, the retry will silently stop working.
            if (std::string(first_error).find("kid not found") == std::string::npos) {
                throw AuthError(AuthErrorCode::InvalidToken, first_error);
            }

            AUTH_LOG(UDA_LOG_DEBUG,
                "Auth: kid not found in cached JWKS, refreshing (possible key rotation)\n");
            try {
                const std::string fresh = fetch_jwks_cached(jwks_uri_, fetcher_, true);
                return verify_with_jwks(decoded, fresh);
            } catch (const AuthError&) {
                throw;
            } catch (const std::exception& e2) {
                throw AuthError(AuthErrorCode::InvalidToken, e2.what());
            }
        }
    }

private:
    // Verify decoded token against a given JWKS JSON string.
    // Throws AuthError(InvalidConfig) for bad algorithm config, AuthError(InvalidToken) for
    // claim mismatches; lets jwt-cpp std::exceptions propagate as-is for the kid-retry logic.
    [[nodiscard]] PayloadType verify_with_jwks(
        const jwt::decoded_jwt<jwt::traits::kazuho_picojson>& decoded,
        const std::string& jwks_json) const
    {
        const auto jwk = jwt::parse_jwks(jwks_json).get_jwk(decoded.get_key_id());
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWK key found (kid=%s)\n", decoded.get_key_id().c_str());

        std::string pub_key_pem;
        const auto x5c = jwk.get_x5c_key_value();
        if (!x5c.empty()) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using x5c component for signature verification\n");
            pub_key_pem = jwt::helper::convert_base64_der_to_pem(x5c);
        } else {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: using RSA n+e components for signature verification\n");
            const auto modulus  = jwk.get_jwk_claim("n").as_string();
            const auto exponent = jwk.get_jwk_claim("e").as_string();
            pub_key_pem = jwt::helper::create_public_key_from_rsa_components(modulus, exponent);
        }

        const auto leeway = static_cast<size_t>(
            cfg_.clock_skew_seconds > 0 ? cfg_.clock_skew_seconds : 0);
        auto verifier = jwt::verify().leeway(leeway);

        // jwt-cpp always validates exp and nbf when present — this is correct behaviour
        // and there is no supported way to disable it.

        bool any_alg = false;
        for (const auto& alg : cfg_.allowed_algs) {
            if (alg == "RS256") {
                verifier.allow_algorithm(jwt::algorithm::rs256(pub_key_pem, "", "", ""));
                any_alg = true;
            } else if (alg == "RS384") {
                verifier.allow_algorithm(jwt::algorithm::rs384(pub_key_pem, "", "", ""));
                any_alg = true;
            } else if (alg == "RS512") {
                verifier.allow_algorithm(jwt::algorithm::rs512(pub_key_pem, "", "", ""));
                any_alg = true;
            } else {
                AUTH_LOG(UDA_LOG_WARN,
                    "Auth: unsupported algorithm '%s' in UDA_SERVER_OIDC_ALLOWED_ALGS "
                    "(supported: RS256, RS384, RS512)\n", alg.c_str());
            }
        }
        if (!any_alg) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "No supported algorithms configured in UDA_SERVER_OIDC_ALLOWED_ALGS; "
                "supported: RS256, RS384, RS512");
        }

        if (cfg_.verify_issuer && !cfg_.issuer.empty()) {
            verifier.with_issuer(cfg_.issuer);
        }
        if (cfg_.verify_audience && !cfg_.audience.empty()) {
            verifier.with_audience(cfg_.audience);
        }

        verifier.verify(decoded);
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWT signature and claims verified\n");

        // Parse payload from the base64url-decoded JSON string using nlohmann/json,
        // avoiding picojson types entirely. String claims are stored as plain strings;
        // arrays, objects, and numbers are JSON-serialised so ClaimPolicy can parse them.
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
    std::string jwks_uri_;
};

} // namespace detail

// -------------------------------------------------------------------------
// authenticate() — testable two-argument form

PayloadType authenticate(const std::string& token,
                         const OidcConfig& cfg,
                         const HttpFetcher& fetcher)
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

    const detail::OidcCTX ctx(cfg, fetcher);
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
// authenticate() — production entry point: reads env, uses cURL

PayloadType authenticate(const std::string& token)
{
    return authenticate(token, OidcConfig::from_env(), curl_http_fetch);
}

} // namespace authentication
} // namespace uda
