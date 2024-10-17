#ifndef UDA_LEGACY_ACCAPI_H
#define UDA_LEGACY_ACCAPI_H

#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef UDA_CLIENT_FLAGS_API
// #warning "Using legacy API names with redundant \"client_flags\" arguments, these will be deprecated in future"

     LIBRARY_API inline DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags)
    {
        return udaGetCurrentDataBlock();
    }

    LIBRARY_API inline int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags)
    {
        return udaGetCurrentDataBlockIndex();
    }

    LIBRARY_API inline int acc_growIdamDataBlocks(CLIENT_FLAGS* client_flags)
    {
        return udaGrowDataBlocks();
    }

    LIBRARY_API inline int acc_getIdamNewDataHandle(CLIENT_FLAGS* client_flags)
    {
        return udaGetNewDataHandle();
    }

    LIBRARY_API inline void setIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag)
    {
        udaSetClientFlag(flag);
    }

    LIBRARY_API inline void resetIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag)
    {
        udaResetClientFlag(flag);
    }

    LIBRARY_API inline void setIdamProperty(const char* property, CLIENT_FLAGS* client_flags)
    {
        udaSetProperty(property);
    }

    LIBRARY_API inline int getIdamProperty(const char* property, const CLIENT_FLAGS* client_flags)
    {
        return udaGetProperty(property);
    }

    LIBRARY_API inline void resetIdamProperty(const char* property, CLIENT_FLAGS* client_flags)
    {
        udaResetProperty(property);
    }

    LIBRARY_API inline void resetIdamProperties(CLIENT_FLAGS* client_flags)
    {
        udaResetProperties();
    }

    CLIENT_BLOCK saveIdamProperties(const CLIENT_FLAGS* client_flags)
    {
        return udaSaveProperties();
    }

    LIBRARY_API inline void restoreIdamProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags)
    {
        udaRestoreProperties(cb);
    }

    LIBRARY_API inline int getIdamLastHandle(CLIENT_FLAGS* client_flags)
    {
        return udaGetLastHandle();
    }

    LIBRARY_API inline void lockIdamThread(CLIENT_FLAGS* client_flags)
    {
        udaLockThread();
    }

    LIBRARY_API inline void unlockUdaThread(CLIENT_FLAGS* client_flags)
    {
        udaUnlockThread();
    }

    LIBRARY_API inline void freeIdamThread(CLIENT_FLAGS* client_flags)
    {
        udaFreeThread();
    }

#else

    LIBRARY_API inline DATA_BLOCK* acc_getCurrentDataBlock()
    {
        return udaGetCurrentDataBlock();
    }

    LIBRARY_API inline int acc_getCurrentDataBlockIndex()
    {
        return udaGetCurrentDataBlockIndex();
    }

    LIBRARY_API inline int acc_growIdamDataBlocks()
    {
        return udaGrowDataBlocks();
    }

    LIBRARY_API inline int acc_getIdamNewDataHandle()
    {
        return udaGetNewDataHandle();
    }

    LIBRARY_API inline void setIdamClientFlag(unsigned int flag)
    {
        udaSetClientFlag(flag);
    }

    LIBRARY_API inline void resetIdamClientFlag(unsigned int flag)
    {
        udaResetClientFlag(flag);
    }

    LIBRARY_API inline void setIdamProperty(const char* property)
    {
        udaSetProperty(property);
    }

    LIBRARY_API inline int getIdamProperty(const char* property)
    {
        return udaGetProperty(property);
    }

    LIBRARY_API inline void resetIdamProperty(const char* property)
    {
        udaResetProperty(property);
    }

    LIBRARY_API inline void resetIdamProperties()
    {
        udaResetProperties();
    }

    LIBRARY_API inline CLIENT_BLOCK saveIdamProperties()
    {
        return udaSaveProperties();
    }

    LIBRARY_API inline void restoreIdamProperties(CLIENT_BLOCK cb)
    {
        udaRestoreProperties(cb);
    }

    LIBRARY_API inline int getIdamLastHandle()
    {
        return udaGetLastHandle();
    }

    LIBRARY_API inline void lockIdamThread()
    {
        udaLockThread();
    }

    LIBRARY_API inline void unlockUdaThread()
    {
        udaUnlockThread();
    }

    LIBRARY_API inline void freeIdamThread()
    {
        udaFreeThread();
    }

#endif 


#ifdef __cplusplus
}
#endif


#endif // UDA_LEGACY_ACCAPI_H
