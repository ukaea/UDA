
#ifndef IDAM_IDAMERRORLOG_H
#define IDAM_IDAMERRORLOG_H

#include <clientserver/idamStructs.h>

#include <time.h>

#define DATELENGTH	27

void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK idamerrorstack);
void initIdamErrorStack(IDAMERRORSTACK *idamerrorstack);
void initIdamErrorRecords(IDAMERRORSTACK *idamerrorstack);
void printIdamErrorStack(IDAMERRORSTACK idamerrorstack);
void addIdamError(IDAMERRORSTACK *idamerrorstack, int type, const char *location, int code, const char *msg);
void concatIdamError(IDAMERRORSTACK errorstackin, IDAMERRORSTACK *errorstackout);
void closeIdamError(IDAMERRORSTACK *idamerrorstack);

extern IDAMERRORSTACK idamerrorstack;

#endif // IDAM_IDAMERRORLOG_H

