#include "cache.h"

#ifdef NOLIBMEMCACHED

struct IdamCached {};

IDAM_CACHE * idamOpenCache() { return NULL; }
void idamFreeCache() {}
char * idamCacheKey(REQUEST_BLOCK * request_block, ENVIRONMENT environment) { return NULL; }

int idamCacheWrite(IDAM_CACHE * cache, REQUEST_BLOCK * request_block, DATA_BLOCK * data_block, ENVIRONMENT environment) { return 0; }
DATA_BLOCK * idamCacheRead(IDAM_CACHE * cache, REQUEST_BLOCK * request_block, ENVIRONMENT environment) { return NULL; }

#else

#include <libmemcached/memcached.h>
#include <logging/logging.h>
#include <clientserver/protocol.h>
#include <clientserver/initStructs.h>
#include <clientserver/memstream.h>
#include <clientserver/xdrlib.h>

#define HASHXDR 1
#ifdef HASHXDR
#  define PARTBLOCKINIT        1
#  define PARTBLOCKUPDATE      2
#  define PARTBLOCKOUTPUT      3

void sha1Block(unsigned char* block, size_t blockSize, unsigned char* md);

void sha1PartBlock(unsigned char* partBlock, size_t partBlockSize, unsigned char* md, unsigned int state);

int sha1File(char* name, FILE* fh, unsigned char* md);

#endif // HASHXDR

#define MAXELEMENTSHA1 20

struct IdamCache {
    memcached_st memcache;
};

static IDAM_CACHE* global_cache = NULL;    // scope limited to this code module

IDAM_CACHE* idamOpenCache()
{
    IDAM_CACHE* cache = malloc(sizeof(IDAM_CACHE));
    memcached_return_t rc;
    memcached_server_st* servers;

    const char* host = getenv("UDA_CACHE_HOST");   // Overrule the default settings
    const char* port = getenv("UDA_CACHE_PORT");

    if (host == NULL && port == NULL) {
        servers = memcached_server_list_append(NULL, IDAM_CACHE_HOST, (in_port_t)IDAM_CACHE_PORT, &rc);
    } else if (host != NULL && port != NULL) {
        servers = memcached_server_list_append(NULL, host, (in_port_t)atoi(port), &rc);
    } else if (host != NULL) {
        servers = memcached_server_list_append(NULL, host, (in_port_t)IDAM_CACHE_PORT, &rc);
    } else {
        servers = memcached_server_list_append(NULL, IDAM_CACHE_HOST, (in_port_t)atoi(port), &rc);
    }

    //memcached_create(&cache->memcache);       // Causes a segmentation Violation!
    cache->memcache = *memcached_create(NULL);
    rc = memcached_server_push(&cache->memcache, servers);

    if (rc == MEMCACHED_SUCCESS) {
        IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", "Added server successfully");
    } else {
        IDAM_LOGF(UDA_LOG_DEBUG, "Couldn't add server: %s\n", memcached_strerror(&cache->memcache, rc));
        free(cache);
        return NULL;
    }

    global_cache = cache;   // Copy the pointer
    return cache;
}

void idamFreeCache() // Will be called by the idamFreeAll function
{
    memcached_free(&global_cache->memcache);
}

// Use the requested signal and source with client specified properties to create a unique key
// All parameters that may effect the data state, e.g. flags, host, port, properties, etc. must be included in the key
// There is a 250 character limit - use SHA1 hash if it exceeds 250
// The local cache should only be used to record data returned from a server after a GET method - Note: Put methods may be disguised in a GET call!
// How to validate the cached data?

char* idamCacheKey(REQUEST_BLOCK* request_block, ENVIRONMENT environment)
{
    // Check Client Properties for permission and requested method
    if (!(clientFlags & CLIENTFLAG_CACHE)) {
        return NULL;
    }

    // **** TODO **** if(!(clientFlags & CLIENTFLAG_CACHE) || request_block->put) return NULL;
    char* delimiter = "&&";
    size_t len = strlen(request_block->source) + strlen(request_block->signal) +
                 strlen(environment.server_host) + 128;
    char* key = (char*)malloc(len * sizeof(char));
    sprintf(key, "%s%s%s%s%s%s%d%s%d%s%d", request_block->signal, delimiter, request_block->source, delimiter,
            environment.server_host, delimiter, environment.server_port, delimiter, environment.clientFlags, delimiter,
            privateFlags);

    // *** TODO: Add server properties (set by the client) to the key - planned to use clientFlags (bit settings) but not implemented yet! ***
    // *** which server is the client connected to .... may not be the default in the ENVIRONMENT structure! - Investigate! ***
    // *** privateFlags is a global also in the CLIENT_BLOCK structure passed to the server (with clientFlags)

    if (len < 250) {
        return key;
    }

#ifndef HASHXDR
    free(key);
    return NULL;
    // No Hash function to create the key
#else
    // Need a compact hash - use SHA1 as always 20 bytes (40 bytes when printable)
    unsigned char md[MAXELEMENTSHA1 + 1];      // SHA1 Hash
    md[MAXELEMENTSHA1] = '\0';
    strcpy((char*)md, "                    ");
    sha1Block((unsigned char*)key, len, md);
    // Convert to a printable string (40 characters) for the key (is this necessary?)
    int j;
    key[40] = '\0';

    for (j = 0; j < 20; j++) {
        sprintf(&key[2 * j], "%2.2x", md[j]);
    }

    return key;
#endif
}

// Use NON-BLOCKING IO mode for performance?
// Write only with the server's permission - which information should be kept in the cache is the concern of the server only
// All data services should indicate whether or not the data returned is suitable for client side caching (all server plugin get methods must decide!)
// The server should also set a recommmended expiration time (lifetime of the stored object) - overridden by the client if necessary

int idamCacheWrite(IDAM_CACHE* cache, REQUEST_BLOCK* request_block, DATA_BLOCK* data_block, ENVIRONMENT environment)
{
#ifdef CACHEDEV

    if (!data_block->cachePermission) {
        return -1;    // Test permission for the Client to cache this structure.
    }

#endif
    char* key = idamCacheKey(request_block, environment);
    IDAM_LOGF(UDA_LOG_DEBUG, "Caching value for key: %s\n", key);

    if (key == NULL) {
        return -1;
    }

    char* buffer;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_ENCODE);

    int token;

    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_SEND, &token, (void*)data_block);
    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
    fclose(memfile);

    // Expiration of the object
    static unsigned int age_max = IDAM_CACHE_EXPIRY;
    static int init = 1;

    if (init) {
        char* env = getenv("UDA_CACHE_EXPIRY");

        if (env != NULL) {
            age_max = (unsigned int)atoi(env);
        }

        init = 0;
    }

    time_t life = time(NULL);
#ifdef CACHEDEV

    if (data_block->cacheExpiryTime > 0) {  // Object expiration time is set by the server
        life += data_block->cacheExpiryTime; // Add a lifetime for the object to the current time
    } else {
        life += age_max;             // Add the default or client overridden lifetime for the object to the current time
    }

#else
    life += age_max;                // Add the default or client overridden lifetime for the object to the current time
#endif
    memcached_return_t rc = memcached_set(&cache->memcache, key, strlen(key), buffer, bufsize, life, (uint32_t)0);

    if (rc != MEMCACHED_SUCCESS) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Couldn't store key: %s\n", memcached_strerror(&cache->memcache, rc));
        free(key);
        return -1;
    }

    rc = memcached_flush_buffers(&cache->memcache);

    if (rc != MEMCACHED_SUCCESS) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Couldn't flush buffers: %s\n", memcached_strerror(&cache->memcache, rc));
        free(key);
        return -1;
    }

    free(key);
    return 0;
}

DATA_BLOCK* idamCacheRead(IDAM_CACHE* cache, REQUEST_BLOCK* request_block, ENVIRONMENT environment)
{
    char* key = idamCacheKey(request_block, environment);
    IDAM_LOGF(UDA_LOG_DEBUG, "Retrieving value for key: %s\n", key);

    if (key == NULL) {
        return NULL;
    }

    memcached_return rc;
    size_t len = 0;
    u_int32_t flags = 0;
    char* value = NULL;
    value = (char*)memcached_get(&cache->memcache, key, strlen(key), &len, &flags, &rc);

    if (rc != MEMCACHED_SUCCESS) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Couldn't retrieve key: %s\n", memcached_strerror(&cache->memcache, rc));
        free(key);
        return NULL;
    }

    char* buffer;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    fwrite(value, sizeof(char), len, memfile);
    fseek(memfile, 0L, SEEK_SET);

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_DECODE);

    DATA_BLOCK* data_block = (DATA_BLOCK*)malloc(sizeof(DATA_BLOCK));
    initDataBlock(data_block);

    int token;
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_RECEIVE, &token, (void*)data_block);

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
    fclose(memfile);
    free(key);

    return data_block;
}

#endif // NOLIBMEMCACHED
