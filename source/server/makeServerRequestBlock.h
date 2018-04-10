#ifndef IDAM_SERVER_MAKESERVERREQUESTBLOCK_H
#define IDAM_SERVER_MAKESERVERREQUESTBLOCK_H

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SERVERBUILD) || defined(FATCLIENT)

void extractFunctionName(char *str, REQUEST_BLOCK *request_block);
void initServerRequestBlock(REQUEST_BLOCK *str);
int makeServerRequestBlock(REQUEST_BLOCK *request_block, PLUGINLIST pluginList);
int sourceFileFormatTest(const char *source, REQUEST_BLOCK *request_block, PLUGINLIST pluginList);
int genericRequestTest(const char *source, REQUEST_BLOCK *request_block, PLUGINLIST pluginList);
int extractArchive(REQUEST_BLOCK *request_block, int reduceSignal);

#endif

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

#endif // IDAM_SERVER_MAKESERVERREQUESTBLOCK_H
