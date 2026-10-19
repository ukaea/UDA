#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace uda {
namespace authentication {

struct JwksEntry {
    std::string                          json_str;
    std::chrono::steady_clock::time_point fetched_at;
};

// Process-global JWKS cache. Mutex-protected. TTL defaults to 300 s
// (overridable via UDA_SERVER_OIDC_JWKS_CACHE_TTL).
// discovery_uris maps discovery URL -> jwks_uri; entries are stable for
// the process lifetime so no TTL is applied to them.
//
// Inject an explicit JwksCache into authenticate() for test isolation — each
// test can construct a fresh cache and avoid cross-test interference.
struct JwksCache {
    std::mutex mutex;
    std::unordered_map<std::string, JwksEntry>  entries;
    std::unordered_map<std::string, std::string> discovery_uris;
    int ttl_seconds;

    JwksCache();
    explicit JwksCache(int ttl_s) : ttl_seconds(ttl_s) {}

    void clear();
};

JwksCache& global_jwks_cache();

} // namespace authentication
} // namespace uda
