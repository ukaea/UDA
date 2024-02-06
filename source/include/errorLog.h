#ifndef UDA_CLIENTSERVER_ERRORLOG_H
#define UDA_CLIENTSERVER_ERRORLOG_H

#include <uda/export.h>
#include "udaStructs.h"
#include <time.h>

#define UDA_DATE_LENGTH 27

//--------------------------------------------------------
// Error Management

#define UDA_SYSTEM_ERROR_TYPE 1
#define UDA_CODE_ERROR_TYPE 2
#define UDA_PLUGIN_ERROR_TYPE 3

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaNumErrors(void);
LIBRARY_API void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack);
LIBRARY_API void udaInitErrorStack(void);
LIBRARY_API void udaInitErrorRecords(const UDA_ERROR_STACK* errorstack);
LIBRARY_API void udaPrintErrorStack(void);
LIBRARY_API void udaAddError(int type, const char* location, int code, const char* msg);
LIBRARY_API void udaConcatError(UDA_ERROR_STACK* errorstackout);
LIBRARY_API void udaFreeErrorStack(UDA_ERROR_STACK* errorstack);
LIBRARY_API void udaCloseError(void);

#ifdef __cplusplus
}
#endif

#define UDA_ADD_ERROR(ERR, MSG) udaAddError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) udaAddError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG)                                                                                      \
    udaAddError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG);                                                              \
    return ERR;

#endif // UDA_CLIENTSERVER_ERRORLOG_H
