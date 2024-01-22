#ifndef UDA_CLIENTSERVER_PRINTSTRUCTS_H
#define UDA_CLIENTSERVER_PRINTSTRUCTS_H

#include "export.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void printRequestData(REQUEST_DATA str);
LIBRARY_API void printRequestBlock(REQUEST_BLOCK str);
LIBRARY_API void printClientBlock(CLIENT_BLOCK str);
LIBRARY_API void printServerBlock(SERVER_BLOCK str);
LIBRARY_API void printDataBlockList(DATA_BLOCK_LIST str);
LIBRARY_API void printDataBlock(DATA_BLOCK str);
LIBRARY_API void printSystemConfig(SYSTEM_CONFIG str);
LIBRARY_API void printDataSystem(DATA_SYSTEM str);
LIBRARY_API void printDataSource(DATA_SOURCE str);
LIBRARY_API void printSignal(SIGNAL str);
LIBRARY_API void printSignalDesc(SIGNAL_DESC str);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PRINTSTRUCTS_H
