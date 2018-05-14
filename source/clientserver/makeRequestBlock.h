#ifndef UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
#define UDA_CLIENSERVER_MAKEREQUESTBLOCK_H

#include <plugins/pluginStructs.h>

#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXMAPDEPTH     10  // Maximum number of chained signal name mappings (Recursive depth)
#define MAXREQDEPTH     4   // Maximum number of Device Name to Server Protocol and Host substitution

int makeRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment);
void extractFunctionName(char* str, REQUEST_BLOCK* request_block);
int sourceFileFormatTest(const char* source, REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment);
int genericRequestTest(const char* source, REQUEST_BLOCK* request_block, PLUGINLIST pluginList);
int extractArchive(REQUEST_BLOCK* request_block, int reduceSignal, const ENVIRONMENT* environment);
void expandEnvironmentVariables(REQUEST_BLOCK *request_block);
int extractSubset(REQUEST_BLOCK *request_block);
void freeNameValueList(NAMEVALUELIST *nameValueList);
void parseNameValue(char *pair, NAMEVALUE *nameValue,unsigned short strip);
int nameValuePairs(char *pairList, NAMEVALUELIST *nameValueList, unsigned short strip);
int nameValueSubstitution(NAMEVALUELIST* nameValueList, char *tpass);
void embeddedValueSubstitution(NAMEVALUELIST* nameValueList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENSERVER_MAKEREQUESTBLOCK_H
