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

void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK idamerrorstack);
void initIdamErrorStack(IDAMERRORSTACK *idamerrorstack);
void initIdamErrorRecords(IDAMERRORSTACK *idamerrorstack);
void printIdamErrorStack(IDAMERRORSTACK idamerrorstack);
void addIdamError(IDAMERRORSTACK *idamerrorstack, int type, const char *location, int code, const char *msg);
void concatIdamError(IDAMERRORSTACK errorstackin, IDAMERRORSTACK *errorstackout);
void closeIdamError(IDAMERRORSTACK *idamerrorstack);

extern IDAMERRORSTACK idamerrorstack;

#define THROW_ERROR(ERR, MSG) addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_CLIENTSERVER_IDAMERRORLOG_H