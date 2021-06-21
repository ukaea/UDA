#ifndef UDA_CLIENTSERVER_INITSTRUCTS_H
#define UDA_CLIENTSERVER_INITSTRUCTS_H

#include "udaStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void initNameValueList(NAMEVALUELIST* nameValueList);
LIBRARY_API void initRequestData(REQUEST_DATA* str);
LIBRARY_API void initRequestBlock(REQUEST_BLOCK* str);
LIBRARY_API void initClientBlock(CLIENT_BLOCK* str, int version, const char* clientname);
LIBRARY_API void initServerBlock(SERVER_BLOCK* str, int version);
LIBRARY_API void initDataBlock(DATA_BLOCK* str);
LIBRARY_API void initDimBlock(DIMS* str);
LIBRARY_API void initDataSystem(DATA_SYSTEM* str);
LIBRARY_API void initSystemConfig(SYSTEM_CONFIG* str);
LIBRARY_API void initDataSource(DATA_SOURCE* str);
LIBRARY_API void initSignal(SIGNAL* str);
LIBRARY_API void initSignalDesc(SIGNAL_DESC* str);
LIBRARY_API void initIdamPutDataBlock(PUTDATA_BLOCK* str);
LIBRARY_API void initPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_INITSTRUCTS_H
