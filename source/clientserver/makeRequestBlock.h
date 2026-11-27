#ifndef UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
#define UDA_CLIENSERVER_MAKEREQUESTBLOCK_H

#include <plugins/pluginStructs.h>

#include "udaStructs.h"
#include "udaDiagnostics.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Request parsing. These report errors and traces only through the diagnostics context they are given (see
// clientserver/udaDiagnostics.h); plugins pass udaPluginDiagnostics(plugin_interface).

LIBRARY_API int udaMakeRequestBlock(const UDA_DIAGNOSTICS* diag, REQUEST_BLOCK* request_block, PLUGINLIST pluginList,
                                    const ENVIRONMENT* environment);
LIBRARY_API int udaMakeRequestData(const UDA_DIAGNOSTICS* diag, REQUEST_DATA* request, PLUGINLIST pluginList,
                                   const ENVIRONMENT* environment);
LIBRARY_API int udaNameValuePairs(const UDA_DIAGNOSTICS* diag, const char* pairList, NAMEVALUELIST* nameValueList,
                                  unsigned short strip);
LIBRARY_API void udaExpandEnvironmentVariables(const UDA_DIAGNOSTICS* diag, char* path);
LIBRARY_API void freeNameValueList(NAMEVALUELIST* nameValueList);

#ifndef UDA_COMMON_BUILD
// Legacy forms: report through the calling library's own error stack and logger (udaGlobalDiagnostics())
LIBRARY_API int make_request_block(REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment);
LIBRARY_API int makeRequestData(REQUEST_DATA* request, PLUGINLIST pluginList, const ENVIRONMENT* environment);
LIBRARY_API int name_value_pairs(const char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip);
LIBRARY_API void expand_environment_variables(char* path);
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
