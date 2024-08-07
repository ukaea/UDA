#include "legacy_accAPI.h"
#include "accAPI.h"

#ifdef UDA_CLIENT_FLAGS_API

    DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags)
    {
        return udaGetCurrentDataBlock();
    }

    int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags)
    {
        return udaGetCurrentDataBlockIndex();
    }

    int acc_growIdamDataBlocks(CLIENT_FLAGS* client_flags)
    {
        return udaGrowDataBlocks();
    }

    int acc_getIdamNewDataHandle(CLIENT_FLAGS* client_flags)
    {
        return udaGetNewDataHandle();
    }

    void setIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag)
    {
        udaSetClientFlag(flag);
    }

    void resetIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag)
    {
        udaResetClientFlag(flag);
    }

    void setIdamProperty(const char* property, CLIENT_FLAGS* client_flags)
    {
        udaSetProperty(property);
    }

    int getIdamProperty(const char* property, const CLIENT_FLAGS* client_flags)
    {
        return udaGetProperty(property);
    }

    void resetIdamProperty(const char* property, CLIENT_FLAGS* client_flags)
    {
        udaResetProperty(property);
    }

    void resetIdamProperties(CLIENT_FLAGS* client_flags)
    {
    udaResetProperties();
    }

    CLIENT_BLOCK saveIdamProperties(const CLIENT_FLAGS* client_flags)
    {
        return udaSaveProperties();
    }

    void restoreIdamProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags)
    {
        udaRestoreProperties(cb);
    }

    int getIdamLastHandle(CLIENT_FLAGS* client_flags)
    {
        return udaGetLastHandle();
    }

    void lockIdamThread(CLIENT_FLAGS* client_flags)
    {
        udaLockThread();
    }

    void unlockUdaThread(CLIENT_FLAGS* client_flags)
    {
        udaUnlockThread();
    }

    void freeIdamThread(CLIENT_FLAGS* client_flags)
    {
        udaFreeThread();
    }

#else

    DATA_BLOCK* acc_getCurrentDataBlock()
    {
        return udaGetCurrentDataBlock();
    }

    int acc_getCurrentDataBlockIndex()
    {
        return udaGetCurrentDataBlockIndex();
    }

    int acc_growIdamDataBlocks()
    {
        return udaGrowDataBlocks();
    }

    int acc_getIdamNewDataHandle()
    {
        return udaGetNewDataHandle();
    }

    void setIdamClientFlag(unsigned int flag)
    {
        udaSetClientFlag(flag);
    }

    void resetIdamClientFlag(unsigned int flag)
    {
        udaResetClientFlag(flag);
    }

    void setIdamProperty(const char* property)
    {
        udaSetProperty(property);
    }

    int getIdamProperty(const char* property)
    {
        return udaGetProperty(property);
    }

    void resetIdamProperty(const char* property)
    {
        udaResetProperty(property);
    }

    void resetIdamProperties()
    {
    udaResetProperties();
    }

    CLIENT_BLOCK saveIdamProperties()
    {
        return udaSaveProperties();
    }

    void restoreIdamProperties(CLIENT_BLOCK cb)
    {
        udaRestoreProperties(cb);
    }

    int getIdamLastHandle()
    {
        return udaGetLastHandle();
    }

    void lockIdamThread()
    {
        udaLockThread();
    }

    void unlockUdaThread()
    {
        udaUnlockThread();
    }

    void freeIdamThread()
    {
        udaFreeThread();
    }

#endif // UDA_LEGACY_API

