#ifndef UDA_CLIENTSERVER_ERRORLOG_H
#define UDA_CLIENTSERVER_ERRORLOG_H

#include <clientserver/udaStructs.h>
#include "export.h"
#include <time.h>

#define DATELENGTH    27

//--------------------------------------------------------
// Error Management

#define SYSTEMERRORTYPE     1
#define CODEERRORTYPE       2
#define PLUGINERRORTYPE     3

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaNumErrors(void);
LIBRARY_API void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack);
LIBRARY_API void initUdaErrorStack(void);
LIBRARY_API void initErrorRecords(const UDA_ERROR_STACK* errorstack);
LIBRARY_API void printIdamErrorStack(void);
LIBRARY_API void addIdamError(int type, const char* location, int code, const char* msg);
LIBRARY_API void concatUdaError(UDA_ERROR_STACK* errorstackout);
LIBRARY_API void freeIdamErrorStack(UDA_ERROR_STACK* errorstack);
LIBRARY_API void closeUdaError(void);

#ifdef __cplusplus
}
#endif

#define ADD_ERROR(ERR, MSG) addIdamError(CODEERRORTYPE, __func__, ERR, MSG)
#define ADD_SYS_ERROR(MSG) addIdamError(SYSTEMERRORTYPE, __func__, errno, MSG)
#define THROW_ERROR(ERR, MSG) addIdamError(CODEERRORTYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_CLIENTSERVER_ERRORLOG_H
