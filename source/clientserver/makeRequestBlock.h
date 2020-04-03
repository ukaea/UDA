#ifndef UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
#define UDA_CLIENSERVER_MAKEREQUESTBLOCK_H

#include <plugins/pluginStructs.h>

#include "udaStructs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAXMAPDEPTH     10  // Maximum number of chained signal name mappings (Recursive depth)
#define MAXREQDEPTH     4   // Maximum number of Device Name to Server Protocol and Host substitution

LIBRARY_API int makeRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment);
LIBRARY_API void extractFunctionName(char* str, REQUEST_BLOCK* request_block);
LIBRARY_API int sourceFileFormatTest(const char* source, REQUEST_BLOCK* request_block, PLUGINLIST pluginList,
                         const ENVIRONMENT* environment);
LIBRARY_API int genericRequestTest(const char* source, REQUEST_BLOCK* request_block);
LIBRARY_API int extractArchive(REQUEST_BLOCK* request_block, int reduceSignal, const ENVIRONMENT* environment);
LIBRARY_API void expandEnvironmentVariables(char* path);
LIBRARY_API int extractSubset(REQUEST_BLOCK* request_block);
LIBRARY_API void freeNameValueList(NAMEVALUELIST* nameValueList);
LIBRARY_API void parseNameValue(char* pair, NAMEVALUE* nameValue, unsigned short strip);
LIBRARY_API int nameValuePairs(char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip);
LIBRARY_API int nameValueSubstitution(NAMEVALUELIST* nameValueList, char* tpass);
LIBRARY_API void embeddedValueSubstitution(NAMEVALUELIST* nameValueList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
