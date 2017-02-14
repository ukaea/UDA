#ifndef IDAM_CLIENTSERVER_IDAMERRORLOG_H
#define IDAM_CLIENTSERVER_IDAMERRORLOG_H

#include <clientserver/idamStructs.h>

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

#endif // IDAM_CLIENTSERVER_IDAMERRORLOG_H

