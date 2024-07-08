#ifndef LEGACY_ACCAPI_H
#define LEGACY_ACCAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define acc_getCurrentDataBlock() acc_getCurrentDataBlock(udaClientFlags())
#define acc_getCurrentDataBlockIndex() acc_getCurrentDataBlockIndex(udaClientFlags())
#define acc_growIdamDataBlocks() acc_growIdamDataBlocks(udaClientFlags()) 
#define acc_getIdamNewDataHandle() acc_getIdamNewDataHandle(udaClientFlags())
#define setIdamClientFlag(flag) setIdamClientFlag(udaClientFlags(), flag)
#define resetIdamClientFlag(flag) resetIdamClientFlag(udaClientFlags(), flag)
#define setIdamProperty(property) setIdamProperty(property, udaClientFlags())
#define getIdamProperty(property) getIdamProperty(property, udaClientFlags())
#define resetIdamProperty(property) resetIdamProperty(property, udaClientFlags())
#define resetIdamProperties() resetIdamProperties(udaClientFlags())
#define saveIdamProperties() saveIdamProperties(udaClientFlags())
#define restoreIdamProperties(cb) restoreIdamProperties(cb, udaClientFlags())
#define getIdamLastHandle() getIdamLastHandle(udaClientFlags())
#define lockIdamThread() lockIdamThread(udaClientFlags())
#define unlockUdaThread() unlockUdaThread(udaClientFlags())
#define freeIdamThread() freeIdamThread(udaClientFlags())

#ifdef __cplusplus
}
#endif


#endif
