#include "jwks_cache.h"
#include <cstdlib>

namespace uda {
namespace authentication {

JwksCache::JwksCache()
{
    const char* ttl_env = std::getenv("UDA_SERVER_OIDC_JWKS_CACHE_TTL");
    int ttl = ttl_env ? std::atoi(ttl_env) : 300;
    ttl_seconds = (ttl > 0) ? ttl : 300;
}

void JwksCache::clear()
{
    std::lock_guard<std::mutex> lock(mutex);
    entries.clear();
    discovery_uris.clear();
}

JwksCache& global_jwks_cache()
{
    static JwksCache cache;
    return cache;
}

} // namespace authentication
} // namespace uda
