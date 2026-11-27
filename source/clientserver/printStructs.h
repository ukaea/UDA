#ifndef UDA_CLIENTSERVER_PRINTSTRUCTS_H
#define UDA_CLIENTSERVER_PRINTSTRUCTS_H

#include "udaStructs.h"
#include "udaDiagnostics.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Debug printers: write through the diagnostics context they are given
LIBRARY_API void udaPrintRequestData(const UDA_DIAGNOSTICS* diag, REQUEST_DATA str);
LIBRARY_API void udaPrintRequestBlock(const UDA_DIAGNOSTICS* diag, REQUEST_BLOCK str);
LIBRARY_API void udaPrintClientBlock(const UDA_DIAGNOSTICS* diag, CLIENT_BLOCK str);
LIBRARY_API void udaPrintServerBlock(const UDA_DIAGNOSTICS* diag, SERVER_BLOCK str);
LIBRARY_API void udaPrintDataBlockList(const UDA_DIAGNOSTICS* diag, DATA_BLOCK_LIST str);
LIBRARY_API void udaPrintDataBlock(const UDA_DIAGNOSTICS* diag, DATA_BLOCK str);
LIBRARY_API void udaPrintSystemConfig(const UDA_DIAGNOSTICS* diag, SYSTEM_CONFIG str);
LIBRARY_API void udaPrintDataSystem(const UDA_DIAGNOSTICS* diag, DATA_SYSTEM str);
LIBRARY_API void udaPrintDataSource(const UDA_DIAGNOSTICS* diag, DATA_SOURCE str);
LIBRARY_API void udaPrintSignal(const UDA_DIAGNOSTICS* diag, SIGNAL str);
LIBRARY_API void udaPrintSignalDesc(const UDA_DIAGNOSTICS* diag, SIGNAL_DESC str);

#ifndef UDA_COMMON_BUILD
// Legacy forms: print through the calling library's own logger
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
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PRINTSTRUCTS_H

