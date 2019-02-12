#ifndef IDAM_CACHE_IDAMCACHE_H
#define IDAM_CACHE_IDAMCACHE_H

#include <time.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CACHE_NOT_OPENED    1
#define CACHE_NOT_AVAILABLE 2
#define CACHE_AVAILABLE     3

#define IDAM_CACHE_HOST     "localhost"     // Override these with environment variables with the same name
#define IDAM_CACHE_PORT     11211
#define IDAM_CACHE_EXPIRY   86400           //24*3600       // Lifetime of the object in Secs

// Cache permissions

#define PLUGINNOTOKTOCACHE  0   // Plugin state management incompatible with client side cacheing
#define PLUGINOKTOCACHE     1   // Data are OK for the Client to Cache

#define PLUGINCACHEDEFAULT  PLUGINOKTOCACHE // The cache permission to use as the default

#define PLUGINNOCACHETYPE   0
#define PLUGINMEMCACHETYPE  1
#define PLUGINFILECACHETYPE 2

typedef struct IdamCache IDAM_CACHE;

IDAM_CACHE* idamOpenCache();

void idamFreeCache();

char* idamCacheKey(const REQUEST_BLOCK* request_block, ENVIRONMENT environment);

int idamCacheWrite(IDAM_CACHE* cache, const REQUEST_BLOCK* request_block, DATA_BLOCK* data_block,
                   LOGMALLOCLIST* logmalloclist,
                   USERDEFINEDTYPELIST* userdefinedtypelist, ENVIRONMENT environment);

DATA_BLOCK* idamCacheRead(IDAM_CACHE* cache, const REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist, ENVIRONMENT environment);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CACHE_IDAMCACHE_H
