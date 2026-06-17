#include "oauth_authentication.h"
#include "oidc_config.h"
#include "claim_policy.h"
#include "authLog.h"

#include <stdexcept>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

#include "logging/logging.h"

namespace uda {
namespace authentication {
namespace detail {

using json = nlohmann::json;

size_t write_callback(void* contents, const size_t size, const size_t count, std::string* output)
{
    const size_t total = size * count;
    output->append(static_cast<char*>(contents), total);
    return total;
}

// Thin OIDC discovery/JWKS layer.  All jwt-cpp and curl details stay in here.
class OidcCTX {
public:
    explicit OidcCTX(const OidcConfig& cfg) : cfg_(cfg)
    {
        try {
            if (!cfg_.jwks_uri.empty()) {
                // Direct JWKS fetch — no discovery
                AUTH_LOG(UDA_LOG_DEBUG, "Auth: fetching JWKS directly from %s\n", cfg_.jwks_uri.c_str());
                jwks_ = get_json(cfg_.jwks_uri);
            } else {
                // OIDC discovery
                if (cfg_.issuer.empty()) {
                    throw std::runtime_error(
                        "OIDC issuer is not configured; set UDA_SERVER_OIDC_ISSUER "
                        "(or legacy UDA_SERVER_KEYCLOAK_REALM)");
                }
                const std::string discovery_url = cfg_.issuer + "/.well-known/openid-configuration";
                AUTH_LOG(UDA_LOG_DEBUG, "Auth: fetching OIDC discovery document from %s\n",
                         discovery_url.c_str());
                const json discovery = get_json(discovery_url);

                // Validate discovered issuer matches configured issuer
                if (discovery.contains("issuer")) {
                    const std::string discovered_issuer = discovery["issuer"];
                    if (cfg_.verify_issuer && discovered_issuer != cfg_.issuer) {
                        throw std::runtime_error(
                            "OIDC discovery issuer mismatch: configured '" + cfg_.issuer +
                            "' but discovery returned '" + discovered_issuer + "'");
                    }
                }

                const std::string jwks_uri = discovery.value("jwks_uri", "");
                if (jwks_uri.empty()) {
                    throw std::runtime_error("OIDC discovery document missing jwks_uri");
                }
                AUTH_LOG(UDA_LOG_DEBUG, "Auth: fetching JWKS from %s\n", jwks_uri.c_str());
                jwks_ = get_json(jwks_uri);
            }
        } catch (const std::exception& e) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: OIDC context creation failed: %s\n", e.what());
            throw;
        }
    }

    // Verify token signature and apply configured validation checks.
    // Returns decoded payload claims as a flat string map.
    // Never logs or exposes the raw token.
    [[nodiscard]] PayloadType verify_token(const std::string& token) const
    {
        try {
            const auto decoded = jwt::decode(token);

            // Look up the signing key by kid
            const auto jwk = jwt::parse_jwks(jwks_.dump()).get_jwk(decoded.get_key_id());
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWK key found (kid=%s)\n", decoded.get_key_id().c_str());

            // Build the RSA public key (from x5c or n+e components)
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

            // Build verifier: algorithm(s) + time checks
            // jwt-cpp always checks exp, nbf, iat when present; leeway is configurable.
            // verify_expiry/verify_nbf toggles are noted but jwt-cpp doesn't support disabling them
            // without patching the library — log a warning if user tries to disable.
            if (!cfg_.verify_expiry) {
                AUTH_LOG(UDA_LOG_WARN,
                    "Auth: UDA_SERVER_OIDC_VERIFY_EXPIRY=0 is set but jwt-cpp always verifies "
                    "exp when present; the setting has no effect\n");
            }
            if (!cfg_.verify_nbf) {
                AUTH_LOG(UDA_LOG_WARN,
                    "Auth: UDA_SERVER_OIDC_VERIFY_NBF=0 is set but jwt-cpp always verifies "
                    "nbf when present; the setting has no effect\n");
            }

            const auto leeway = static_cast<size_t>(
                cfg_.clock_skew_seconds > 0 ? cfg_.clock_skew_seconds : 0);

            auto verifier = jwt::verify().leeway(leeway);

            // Add allowed algorithms (default RS256; all RSA variants share the same key)
            bool any_alg_added = false;
            for (const auto& alg : cfg_.allowed_algs) {
                if (alg == "RS256") {
                    verifier.allow_algorithm(jwt::algorithm::rs256(pub_key_pem, "", "", ""));
                    any_alg_added = true;
                } else if (alg == "RS384") {
                    verifier.allow_algorithm(jwt::algorithm::rs384(pub_key_pem, "", "", ""));
                    any_alg_added = true;
                } else if (alg == "RS512") {
                    verifier.allow_algorithm(jwt::algorithm::rs512(pub_key_pem, "", "", ""));
                    any_alg_added = true;
                } else {
                    AUTH_LOG(UDA_LOG_WARN, "Auth: unsupported algorithm '%s' in UDA_SERVER_OIDC_ALLOWED_ALGS "
                             "(only RS256/RS384/RS512 currently supported)\n", alg.c_str());
                }
            }
            if (!any_alg_added) {
                throw std::runtime_error(
                    "No supported algorithms in UDA_SERVER_OIDC_ALLOWED_ALGS; "
                    "supported: RS256, RS384, RS512");
            }

            // Conditional standard claim checks
            if (cfg_.verify_issuer && !cfg_.issuer.empty()) {
                verifier.with_issuer(cfg_.issuer);
            }
            if (cfg_.verify_audience && !cfg_.audience.empty()) {
                verifier.with_audience(cfg_.audience);
            }

            verifier.verify(decoded);
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: JWT signature and standard claim checks passed\n");

            // Convert picojson payload to flat string map
            PayloadType payload_map;
            for (const auto& kv : decoded.get_payload_json()) {
                payload_map.emplace(kv.first, kv.second.to_str());
            }
            return payload_map;

        } catch (const std::exception& e) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: token verification failed: %s\n", e.what());
            throw;
        }
    }

private:
    static json get_json(const std::string& url)
    {
        const CurlWrapper curl;
        return json::parse(curl.perform_get_request(url));
    }

    const OidcConfig& cfg_;
    json jwks_;
};

} // namespace detail

// -------------------------------------------------------------------------
// CurlWrapper

CurlWrapper::CurlWrapper()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    handle_ = curl_easy_init();
    if (!handle_) {
        throw std::runtime_error("Failed to initialise cURL");
    }
}

CurlWrapper::~CurlWrapper()
{
    if (handle_) curl_easy_cleanup(handle_);
    curl_global_cleanup();
}

std::string CurlWrapper::perform_get_request(const std::string& url) const
{
    std::string response;
    try {
        curl_easy_setopt(handle_, CURLOPT_URL, url.c_str());
        set_common_options(&response);
        const CURLcode rc = curl_easy_perform(handle_);
        handle_curl_response(rc);
    } catch (const std::exception& e) {
        handle_error(e);
    }
    return response;
}

void CurlWrapper::set_common_options(std::string* response_data) const
{
    curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, detail::write_callback);
    curl_easy_setopt(handle_, CURLOPT_WRITEDATA, response_data);
}

void CurlWrapper::handle_curl_response(const CURLcode response)
{
    if (response != CURLE_OK) {
        throw std::runtime_error(
            std::string("curl_easy_perform() failed: ") + curl_easy_strerror(response));
    }
}

void CurlWrapper::handle_error(const std::exception& e)
{
    UDA_LOG(UDA_LOG_ERROR, "CURL error: %s\n", e.what());
    throw;
}

// -------------------------------------------------------------------------
// authenticate()

PayloadType authenticate(const std::string& token)
{
    if (token.empty()) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: token validation failed — token is empty\n");
        throw std::runtime_error("Authentication is enabled but no bearer token was provided");
    }

    const OidcConfig cfg = OidcConfig::from_env();

    if (cfg.issuer.empty() && cfg.jwks_uri.empty()) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: token validation failed — no issuer or JWKS URI configured; "
            "set UDA_SERVER_OIDC_ISSUER (or legacy UDA_SERVER_KEYCLOAK_REALM)\n");
        throw std::runtime_error(
            "OIDC authentication is enabled but no issuer is configured; "
            "set UDA_SERVER_OIDC_ISSUER or UDA_SERVER_KEYCLOAK_REALM");
    }

    if (cfg.legacy_keycloak_config) {
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: using legacy Keycloak env var config "
            "(UDA_SERVER_KEYCLOAK_REALM / UDA_SERVER_KEYCLOAK_CLIENT_ID); "
            "prefer UDA_SERVER_OIDC_ISSUER / UDA_SERVER_OIDC_CLIENT_ID\n");
    }

    AUTH_LOG(UDA_LOG_DEBUG, "Auth: OIDC issuer=%s client_id=%s audience=%s verify_issuer=%d "
             "verify_audience=%d leeway=%ds\n",
             cfg.issuer.c_str(), cfg.client_id.c_str(), cfg.audience.c_str(),
             cfg.verify_issuer, cfg.verify_audience, cfg.clock_skew_seconds);

    // Verify token signature and standard claims
    const detail::OidcCTX ctx(cfg);
    PayloadType payload = ctx.verify_token(token);

    // Apply legacy azp == client_id check when using Keycloak config and no explicit claim policy.
    // This preserves the behaviour of the original implementation for existing deployments.
    if (cfg.legacy_keycloak_config && !cfg.client_id.empty() && cfg.required_claims.empty()) {
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: applying legacy default claim policy: azp must equal client_id '%s'\n",
            cfg.client_id.c_str());
        auto it = payload.find("azp");
        if (it == payload.end() || it->second != cfg.client_id) {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: legacy claim check failed — azp does not match client_id '%s'\n",
                cfg.client_id.c_str());
            throw std::runtime_error(
                "Token claim 'azp' does not match configured client_id (legacy policy)");
        }
    }

    // Apply configurable claim policy
    if (!cfg.required_claims.empty()) {
        ClaimPolicy policy;
        try {
            policy = ClaimPolicy::parse(cfg.required_claims);
        } catch (const std::exception& e) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: malformed UDA_SERVER_OIDC_REQUIRED_CLAIMS: %s\n", e.what());
            throw std::runtime_error(
                std::string("Malformed UDA_SERVER_OIDC_REQUIRED_CLAIMS: ") + e.what());
        }

        AUTH_LOG(UDA_LOG_DEBUG, "Auth: applying claim policy from UDA_SERVER_OIDC_REQUIRED_CLAIMS\n");
        std::string policy_error;
        if (!policy.check(payload, policy_error)) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: claim policy check failed: %s\n", policy_error.c_str());
            throw std::runtime_error("Token claim policy check failed: " + policy_error);
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: claim policy passed\n");
    }

    return payload;
}

} // namespace authentication
} // namespace uda
