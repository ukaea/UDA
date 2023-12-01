#ifndef UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
#define UDA_CLIENSERVER_MAKEREQUESTBLOCK_H

#include "pluginStructs.h"

#include "udaStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int make_request_block(REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment);
LIBRARY_API int makeRequestData(REQUEST_DATA* request, PLUGINLIST pluginList, const ENVIRONMENT* environment);
LIBRARY_API int name_value_pairs(const char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip);
LIBRARY_API void freeNameValueList(NAMEVALUELIST* nameValueList);
LIBRARY_API void expand_environment_variables(char* path);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
