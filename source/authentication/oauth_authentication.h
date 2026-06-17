#pragma once

#include "auth_error.h"
#include "oidc_config.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace uda {
namespace authentication {

// Decoded JWT payload as a flat string map.
// Nested JSON claims (e.g. realm_access) are preserved as JSON-encoded strings
// and can be traversed by ClaimPolicy using dot-separated paths.
using PayloadType = std::unordered_map<std::string, std::string>;

// HTTP GET function: given a URL, returns the response body or throws on failure.
// The default implementation uses cURL, enforces HTTPS, and sets timeouts.
// Injectable for unit tests via authenticate(token, cfg, fetcher).
using HttpFetcher = std::function<std::string(const std::string& url)>;

// Default HTTP fetcher using cURL.
// Enforces HTTPS unless UDA_SERVER_OIDC_ALLOW_HTTP=1 is set.
// Sets connect timeout (10 s) and total timeout (30 s).
// Follows up to 3 redirects. Fails on HTTP 4xx/5xx.
// Throws AuthError(DiscoveryFailed|JwksFetchFailed, ...) on network or HTTP error.
std::string curl_http_fetch(const std::string& url);

// Validate a bearer token using the provided OIDC config and HTTP fetcher.
// This is the primary testable entry point — inject a mock fetcher in tests.
// Throws AuthError on any validation failure; never logs the raw token value.
// JWKS is cached process-globally with TTL UDA_SERVER_OIDC_JWKS_CACHE_TTL (default 300s).
PayloadType authenticate(const std::string& token,
                         const OidcConfig& cfg,
                         const HttpFetcher& fetcher);

// Convenience wrapper: reads OidcConfig from environment and uses cURL.
PayloadType authenticate(const std::string& token);

} // namespace authentication
} // namespace uda

struct AuthPayload {
    const uda::authentication::PayloadType* auth_payload;
};
