#ifndef UDA_CLIENTSERVER_IDAMERRORLOG_H
#define UDA_CLIENTSERVER_IDAMERRORLOG_H

#include <clientserver/udaStructs.h>

#include <time.h>

#define DATELENGTH	27

//--------------------------------------------------------
// Error Management

#define SYSTEMERRORTYPE     1
#define CODEERRORTYPE       2
#define PLUGINERRORTYPE     3

#ifdef __cplusplus
extern "C" {
#endif

int udaNumErrors(void);
void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK* errorstack);
void initIdamErrorStack(void);
void initIdamErrorRecords(void);
void printIdamErrorStack(void);
void addIdamError(int type, const char* location, int code, const char* msg);
void concatIdamError(IDAMERRORSTACK* errorstackout);
void freeIdamErrorStack(IDAMERRORSTACK* errorstack);
void closeIdamError(void);

#ifdef __cplusplus
}
#endif

#define THROW_ERROR(ERR, MSG) addIdamError(CODEERRORTYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_CLIENTSERVER_IDAMERRORLOG_H
