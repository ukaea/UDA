#pragma once

// The one outbound HTTP GET in UDA.
//
// Deliberately a standalone translation unit: the OIDC server code and the help plugin
// both need it, but they are linked into different libraries, and the UDA client library
// is intentionally free of any libcurl dependency. Keeping it here means there is still
// only one place where UDA's HTTP timeout, redirect and error policy is defined.

#include <string>

namespace uda {
namespace authentication {

// Options for http_get. The defaults are the OIDC discovery/JWKS policy.
struct HttpGetOptions {
    long connect_timeout_ms = 10000;
    long total_timeout_ms   = 30000;
    long max_redirects      = 3;
    // When true, a non-HTTPS URL is rejected outright unless UDA_SERVER_OIDC_ALLOW_HTTP=1.
    // Callers talking to a local, deployment-private service (the help plugin's
    // authorisation endpoint, say) set this false.
    bool require_https      = true;
};

// Fails on HTTP 4xx/5xx, follows redirects up to the configured limit.
// Throws AuthError(InvalidConfig) for a rejected scheme and AuthError(JwksFetchFailed)
// for a network or HTTP failure.
std::string http_get(const std::string& url, const HttpGetOptions& opts = {});

} // namespace authentication
} // namespace uda
