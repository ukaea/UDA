#ifndef UDA_CLIENTSERVER_ERRORLOG_H
#define UDA_CLIENTSERVER_ERRORLOG_H

#include <clientserver/udaStructs.h>
#include <clientserver/udaDiagnostics.h>
#include "export.h"
#include <time.h>

#define UDA_DATE_LENGTH    27

//--------------------------------------------------------
// Error Management
//
// The functions below use the error stack held by the library they are compiled into (the client or the server).
// Code built for the plugin support library (UDA_COMMON_BUILD) has no such state and reports through an explicit
// UDA_DIAGNOSTICS context instead (clientserver/udaDiagnostics.h), so these are not declared for it.

#ifndef UDA_COMMON_BUILD

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaNumErrors(void);
LIBRARY_API void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack);
LIBRARY_API void initUdaErrorStack(void);
LIBRARY_API void printIdamErrorStack(void);
LIBRARY_API void addIdamError(int type, const char* location, int code, const char* msg);
LIBRARY_API void concatUdaError(UDA_ERROR_STACK* errorstackout);

LIBRARY_API void freeIdamErrorStack(UDA_ERROR_STACK* errorstack);
LIBRARY_API void closeUdaError(void);

/**
 * A diagnostics context that forwards to this library's own error stack (addIdamError()) and logger (udaLog()).
 * Used by the client and server when calling shared helpers that take a UDA_DIAGNOSTICS.
 */
LIBRARY_API UDA_DIAGNOSTICS udaGlobalDiagnostics(void);

#ifdef __cplusplus
}
#endif

#define UDA_ADD_ERROR(ERR, MSG) addIdamError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG) addIdamError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_COMMON_BUILD

#endif // UDA_CLIENTSERVER_ERRORLOG_H
