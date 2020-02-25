#ifndef UDA_FILECACHE_H
#define UDA_FILECACHE_H

#include <stdio.h>
#include <time.h>

#include <clientserver/udaStructs.h>

#define CACHE_MAXCOUNT		100		// Max Attempts at obtaining a database table lock
#define CACHE_HOURSVALID	0
#define CACHE_MAXDEADRECORDS	2
#define CACHE_MAXRECORDS	9		// Including comment records
#define CACHE_FIRSTRECORDLENGTH	30
#define CACHE_MAXLOCKTIME	100		// Maximum Grace period after time expiry when a file record is locked
#define CACHE_NORECORD		1
#define CACHE_RECORDFOUND	2
#define CACHE_PURGERECORD	3
#define CACHE_NOFREERECORD	4
#define CACHE_FREERECORD	5

#define CACHE_DEADRECORD	0
#define CACHE_LIVERECORD	1
#define CACHE_LOCKEDRECORD	2		// Do Not purge this file - possibly in use

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// File Cache
extern REQUEST_BLOCK* request_block_ptr;   // Pointer to the User REQUEST_BLOCK

LIBRARY_API int idamClientLockCache(FILE** db, short type);
LIBRARY_API int idamClientCacheTimeValid(unsigned long long timestamp);
LIBRARY_API int idamClientCacheLockedTimeValid(unsigned long long timestamp);
LIBRARY_API int idamClientCacheFileValid(const char* filename);
LIBRARY_API int idamClientGetCacheStats(FILE* db, unsigned long* recordCount, unsigned long* deadCount, unsigned long* endOffset,
                            char csvChar);
LIBRARY_API void idamClientUpdateCacheStats(FILE* db, unsigned long recordCount, unsigned long deadCount, unsigned long endOffset,
                                char csvChar);
LIBRARY_API int idamClientPurgeCache(FILE* db, unsigned long recordCount, unsigned long* endOffset);
LIBRARY_API int idamClientReadCache(DATA_BLOCK* data_block, char* filename, int protocolVersion);
LIBRARY_API int idamClientGetCacheFilename(REQUEST_BLOCK* request_block, char** cacheFilename);
LIBRARY_API int idamClientWriteCache(char* filename);
LIBRARY_API unsigned int xcrc32(const unsigned char* buf, int len, unsigned int init);

#ifdef __cplusplus
}
#endif

#endif // UDA_FILECACHE_H
