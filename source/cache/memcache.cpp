#include "memcache.hpp"

#include <algorithm>
#include <fmt/format.h>

#include "cache.h"

#ifdef NOLIBMEMCACHED

namespace uda
{
namespace cache
{

struct UdaCache {
    int dummy_;
};

} // namespace cache
} // namespace uda

uda::cache::UdaCache* uda::cache::open_cache()
{
    return nullptr;
}

void uda::cache::free_cache() {}

int uda::cache::cache_write(uda::cache::UdaCache* cache, const REQUEST_DATA* request_data, DATA_BLOCK* data_block,
                            LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                            ENVIRONMENT environment, int protocolVersion, uint32_t flags,
                            LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    return 0;
}

DATA_BLOCK* uda::cache::cache_read(uda::cache::UdaCache* cache, const REQUEST_DATA* request_data,
                                   LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   ENVIRONMENT environment, int protocolVersion, uint32_t flags,
                                   LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    return nullptr;
}

#else

#  include <libmemcached/memcached.h>
#  include <openssl/ssl.h>
#  include <sstream>
#  include <string>
#  include <tuple>
// needed for GCC 12.0 - do not delete
#  include <algorithm>

#  include <clientserver/errorLog.h>
#  include <clientserver/initStructs.h>
#  include <clientserver/memstream.h>
#  include <clientserver/xdrlib.h>
#  include <logging/logging.h>

#  define UDA_CACHE_HOST "localhost" // Override these with environment variables with the same name
#  define UDA_CACHE_PORT 11211
#  define UDA_CACHE_EXPIRY 86400 // 24*3600       // Lifetime of the object in Secs

#  define MAX_ELEMENT_SHA1 20

namespace uda
{
namespace cache
{

struct UdaCache {
    memcached_st memcache;
};

} // namespace cache
} // namespace uda

namespace
{

static uda::cache::UdaCache* global_cache = nullptr; // scope limited to this code module

/**
 * Use the requested signal and source with client specified properties to create a unique key.
 *
 * All parameters that may effect the data state, e.g. flags, host, port, properties, etc. must be included in the key.
 *
 * There is a 250 character limit - use SHA1 hash if it exceeds 250. The local cache should only be used to record data
 * returned from a server after a GET method - Note: Put methods may be disguised in a GET call!
 */
std::string generate_cache_key(const REQUEST_DATA* request, ENVIRONMENT environment, uint32_t flags,
                               unsigned int private_flags)
{
    // Check Properties for permission and requested method
    if (!(flags & CLIENTFLAG_CACHE)) {
        return {};
    }

    const char* delimiter = "&&";
    std::stringstream ss;
    ss << request->signal << delimiter << request->source << delimiter << environment.server_host << delimiter
       << environment.server_port << delimiter << flags << delimiter << private_flags;

    auto key = ss.str();
    std::transform(key.begin(), key.end(), key.begin(), [](const decltype(key)::value_type c) {
        if (std::isspace(c)) {
            return '_';
        }
        return static_cast<decltype(key)::value_type>(std::tolower(c));
    });

    if (key.length() < 250) {
        return key;
    }

    // Need a compact hash - use SHA1 as always 20 bytes (40 bytes when printable)
    unsigned char hash[MAX_ELEMENT_SHA1 + 1];
    memset(hash, ' ', MAX_ELEMENT_SHA1);
    hash[MAX_ELEMENT_SHA1] = '\0';
    SHA1(reinterpret_cast<const unsigned char*>(key.data()), key.length(), hash);

    // Convert to a printable string (40 characters) for the key (is this necessary?)
    std::string hash_key;
    hash_key.reserve(40);
    for (int i = 0; i < 20; i++) {
        hash_key += fmt::format("{:2.2x}", hash[i]);
    }

    return hash_key;
}

int memcache_put(uda::cache::UdaCache* cache, const char* key, const char* buffer, size_t bufsize)
{
    // Expiration of the object
    static unsigned int age_max = UDA_CACHE_EXPIRY;
    static bool init = false;

    if (!init) {
        char* env = getenv("UDA_CACHE_EXPIRY");

        if (env != nullptr) {
            age_max = (unsigned int)atoi(env);
        }

        init = true;
    }

    time_t life = time(nullptr);

#  ifdef CACHEDEV
    if (data_block->cacheExpiryTime > 0) {
        // Object expiration time is set by the server
        life += data_block->cacheExpiryTime;
    } else {
        // Add the default or client overridden lifetime for the object to the current time
        life += age_max;
    }
#  else
    // Add the default or client overridden lifetime for the object to the current time
    life += age_max;
#  endif

    memcached_return_t rc = memcached_set(&cache->memcache, key, strlen(key), buffer, bufsize, life, (uint32_t)0);

    if (rc != MEMCACHED_SUCCESS) {
        UDA_THROW_ERROR(-1, memcached_strerror(&cache->memcache, rc));
    }

    rc = memcached_flush_buffers(&cache->memcache);

    if (rc != MEMCACHED_SUCCESS) {
        UDA_THROW_ERROR(-1, memcached_strerror(&cache->memcache, rc));
    }

    return 0;
}

std::pair<char*, size_t> get_cache_value(uda::cache::UdaCache* cache, const char* key)
{
    UDA_LOG(UDA_LOG_DEBUG, "Retrieving value for key: %s\n", key);
    memcached_return rc;
    size_t len = 0;
    u_int32_t flags = 0;
    char* value = memcached_get(&cache->memcache, key, strlen(key), &len, &flags, &rc);

    if (rc != MEMCACHED_SUCCESS) {
        UDA_LOG(UDA_LOG_ERROR, "Couldn't retrieve key: %s\n", memcached_strerror(&cache->memcache, rc));
        return {nullptr, 0};
    }

    return {value, len};
}

} // namespace

uda::cache::UdaCache* uda::cache::open_cache()
{
    static bool init = false;
    if (init) {
        return global_cache;
    }

    auto cache = (uda::cache::UdaCache*)malloc(sizeof(uda::cache::UdaCache));
    memcached_return_t rc;
    memcached_server_st* servers;

    const char* host = getenv("UDA_CACHE_HOST"); // Overrule the default settings
    const char* port = getenv("UDA_CACHE_PORT");

    if (host == nullptr && port == nullptr) {
        servers = memcached_server_list_append(nullptr, UDA_CACHE_HOST, (in_port_t)UDA_CACHE_PORT, &rc);
    } else if (host != nullptr && port != nullptr) {
        servers = memcached_server_list_append(nullptr, host, (in_port_t)atoi(port), &rc);
    } else if (host != nullptr) {
        servers = memcached_server_list_append(nullptr, host, (in_port_t)UDA_CACHE_PORT, &rc);
    } else {
        servers = memcached_server_list_append(nullptr, UDA_CACHE_HOST, (in_port_t)atoi(port), &rc);
    }

    // memcached_create(&cache->memcache);       // Causes a segmentation Violation!
    cache->memcache = *memcached_create(nullptr);
    rc = memcached_server_push(&cache->memcache, servers);

    if (rc == MEMCACHED_SUCCESS) {
        UDA_LOG(UDA_LOG_DEBUG, "%s\n", "Added server successfully");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Couldn't add server: %s\n", memcached_strerror(&cache->memcache, rc));
        free(cache);
        init = true;
        return nullptr;
    }

    global_cache = cache; // Copy the pointer
    init = true;
    return cache;
}

void uda::cache::free_cache() // Will be called by the idamFreeAll function
{
    memcached_free(&global_cache->memcache);
    free(global_cache);
    global_cache = nullptr;
}

int uda::cache::cache_write(uda::cache::UdaCache* cache, const REQUEST_DATA* request_data, DATA_BLOCK* data_block,
                            LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                            ENVIRONMENT environment, int protocolVersion, uint32_t flags,
                            LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
#  ifdef CACHEDEV
    if (!data_block->cachePermission) {
        // Test permission for the Client to cache this structure.
        return -1;
    }
#  endif
    int rc = 0;

    auto key = generate_cache_key(request_data, environment, flags, private_flags);
    UDA_LOG(UDA_LOG_DEBUG, "Caching value for key: %s\n", key.c_str());

    if (key.empty()) {
        return -1;
    }

    char* buffer = nullptr;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    writeCacheData(memfile, logmalloclist, userdefinedtypelist, data_block, protocolVersion, log_struct_list,
                   private_flags, malloc_source);

    rc = memcache_put(cache, key.c_str(), buffer, bufsize);

    fclose(memfile);
    free(buffer);

    return rc;
}

DATA_BLOCK* uda::cache::cache_read(uda::cache::UdaCache* cache, const REQUEST_DATA* request_data,
                                   LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   ENVIRONMENT environment, int protocolVersion, uint32_t flags,
                                   LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    auto key = generate_cache_key(request_data, environment, flags, private_flags);
    if (key.empty()) {
        return nullptr;
    }

    char* value;
    size_t len;
    std::tie(value, len) = get_cache_value(cache, key.c_str());
    if (value == nullptr) {
        return nullptr;
    }

    char* buffer = nullptr;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    fwrite(value, sizeof(char), len, memfile);
    fseek(memfile, 0L, SEEK_SET);

    auto data = readCacheData(memfile, logmalloclist, userdefinedtypelist, protocolVersion, log_struct_list,
                              private_flags, malloc_source);
    fclose(memfile);
    free(buffer);

    return data;
}

#endif // NOLIBMEMCACHED
