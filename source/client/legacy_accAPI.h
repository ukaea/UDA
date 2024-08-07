#ifndef LEGACY_ACCAPI_H
#define LEGACY_ACCAPI_H

#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef UDA_CLIENT_FLAGS_API
// #warning "Using legacy API names with redundant \"client_flags\" arguments, these will be deprecated in future"

    LIBRARY_API DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags);
    LIBRARY_API int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags);
    LIBRARY_API int acc_growIdamDataBlocks(CLIENT_FLAGS* client_flags);
    LIBRARY_API int acc_getIdamNewDataHandle(CLIENT_FLAGS* client_flags);
    LIBRARY_API void setIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);
    LIBRARY_API void resetIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);
    LIBRARY_API void setIdamProperty(const char* property, CLIENT_FLAGS* client_flags);
    LIBRARY_API int getIdamProperty(const char* property, const CLIENT_FLAGS* client_flags);
    LIBRARY_API void resetIdamProperty(const char* property, CLIENT_FLAGS* client_flags);
    LIBRARY_API void resetIdamProperties(CLIENT_FLAGS* client_flags);
    LIBRARY_API CLIENT_BLOCK saveIdamProperties(const CLIENT_FLAGS* client_flags);
    LIBRARY_API void restoreIdamProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags);
    LIBRARY_API int getIdamLastHandle(CLIENT_FLAGS* client_flags);
    LIBRARY_API void lockIdamThread(CLIENT_FLAGS* client_flags);
    LIBRARY_API void unlockUdaThread(CLIENT_FLAGS* client_flags);
    LIBRARY_API void freeIdamThread(CLIENT_FLAGS* client_flags);
#else

    LIBRARY_API DATA_BLOCK* acc_getCurrentDataBlock();
    LIBRARY_API int acc_getCurrentDataBlockIndex();
    LIBRARY_API int acc_growIdamDataBlocks();
    LIBRARY_API int acc_getIdamNewDataHandle();
    LIBRARY_API void setIdamClientFlag(unsigned int flag);
    LIBRARY_API void resetIdamClientFlag(unsigned int flag);
    LIBRARY_API void setIdamProperty(const char* property);
    LIBRARY_API int getIdamProperty(const char* property);
    LIBRARY_API void resetIdamProperty(const char* property);
    LIBRARY_API void resetIdamProperties();
    LIBRARY_API CLIENT_BLOCK saveIdamProperties();
    LIBRARY_API void restoreIdamProperties(CLIENT_BLOCK cb);
    LIBRARY_API int getIdamLastHandle();
    LIBRARY_API void lockIdamThread();
    LIBRARY_API void unlockUdaThread();
    LIBRARY_API void freeIdamThread();

#endif 


#ifdef __cplusplus
}
#endif


#endif // LEGACY_ACCAPI_H
