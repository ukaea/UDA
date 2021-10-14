#ifndef UDA_CACHE_MEMCACHE_H
#define UDA_CACHE_MEMCACHE_H

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

namespace uda {
namespace cache {

struct UdaCache;

UdaCache* udaOpenCache();

void udaFreeCache();

int udaCacheWrite(UdaCache* cache, const REQUEST_DATA* request_data, DATA_BLOCK* data_block,
                  LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                  ENVIRONMENT environment, int protocolVersion, int flags);

DATA_BLOCK* udaCacheRead(UdaCache* cache, const REQUEST_DATA* request_data, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, ENVIRONMENT environment, int protocolVersion,
                         int flags);

}
}

#endif // UDA_CACHE_MEMCACHE_H
