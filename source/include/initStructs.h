#ifndef UDA_CLIENTSERVER_INITSTRUCTS_H
#define UDA_CLIENTSERVER_INITSTRUCTS_H

#include "export.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void udaInitNameValueList(NAMEVALUELIST* nameValueList);
LIBRARY_API void udaInitRequestData(REQUEST_DATA* str);
LIBRARY_API void udaInitRequestBlock(REQUEST_BLOCK* str);
LIBRARY_API void udaInitClientBlock(CLIENT_BLOCK* str, int version, const char* clientname);
LIBRARY_API void udaInitServerBlock(SERVER_BLOCK* str, int version);
LIBRARY_API void udaInitDataBlock(DATA_BLOCK* str);
LIBRARY_API void udaInitDataBlockList(DATA_BLOCK_LIST* str);
LIBRARY_API void udaInitDimBlock(DIMS* str);
LIBRARY_API void udaInitDataSystem(DATA_SYSTEM* str);
LIBRARY_API void udaInitSystemConfig(SYSTEM_CONFIG* str);
LIBRARY_API void udaInitDataSource(DATA_SOURCE* str);
LIBRARY_API void udaInitSignal(SIGNAL* str);
LIBRARY_API void udaInitSignalDesc(SIGNAL_DESC* str);
LIBRARY_API void udaInitPutDataBlock(PUTDATA_BLOCK* str);
LIBRARY_API void udaInitPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_INITSTRUCTS_H
