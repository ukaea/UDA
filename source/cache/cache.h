#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include <time.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CACHE_NOT_OPENED    1
#define CACHE_NOT_AVAILABLE 2
#define CACHE_AVAILABLE     3

#define UDA_CACHE_HOST     "localhost"     // Override these with environment variables with the same name
#define UDA_CACHE_PORT     11211
#define UDA_CACHE_EXPIRY   86400           //24*3600       // Lifetime of the object in Secs

// Cache permissions

#define UDA_PLUGIN_NOT_OK_TO_CACHE  0   // Plugin state management incompatible with client side cacheing
#define UDA_PLUGIN_OK_TO_CACHE      1   // Data are OK for the Client to Cache

#define UDA_PLUGIN_CACHE_DEFAULT  UDA_PLUGIN_NOT_OK_TO_CACHE // The cache permission to use as the default

#define UDA_PLUGIN_NO_CACHE_TYPE   0
#define UDA_PLUGIN_MEM_CACHE_TYPE  1
#define UDA_PLUGIN_FILE_CACHE_TYPE 2

typedef struct UdaCache UDA_CACHE;

UDA_CACHE* idamOpenCache();

void idamFreeCache();

char* idamCacheKey(const REQUEST_BLOCK* request_block, ENVIRONMENT environment);

int idamCacheWrite(UDA_CACHE* cache, const REQUEST_BLOCK* request_block, DATA_BLOCK* data_block,
                   LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, ENVIRONMENT environment);

DATA_BLOCK* idamCacheRead(UDA_CACHE* cache, const REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist, ENVIRONMENT environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_CACHE_H
