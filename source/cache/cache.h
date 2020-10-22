#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include <cstdio>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void writeCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                const DATA_BLOCK* data_block, int protocolVersion);

LIBRARY_API DATA_BLOCK*
readCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_CACHE_H