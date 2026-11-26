#ifndef UDA_CLIENTSERVER_ERRORLOG_H
#define UDA_CLIENTSERVER_ERRORLOG_H

#include <clientserver/udaStructs.h>
#include "export.h"
#include <time.h>

#define UDA_DATE_LENGTH    27

//--------------------------------------------------------
// Error Management

#define UDA_SYSTEM_ERROR_TYPE     1
#define UDA_CODE_ERROR_TYPE       2
#define UDA_PLUGIN_ERROR_TYPE     3

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaNumErrors(void);
LIBRARY_API void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack);
LIBRARY_API void initUdaErrorStack(void);
LIBRARY_API void initErrorRecords(UDA_ERROR_STACK* errorstack);
LIBRARY_API void printIdamErrorStack(void);
LIBRARY_API void addIdamError(int type, const char* location, int code, const char* msg);
LIBRARY_API void concatUdaError(UDA_ERROR_STACK* errorstackout);

/**
 * Append a single error record directly to the given error stack, without touching any library-global state.
 *
 * Strings are copied and terminated, insertion order is preserved and at most UDA_MAX_ERRORS records are kept
 * (earliest first). System errors (UDA_SYSTEM_ERROR_TYPE) have the strerror() text for code appended.
 *
 * @return 0 on success, 1 if the stack was full and the record was dropped, -1 if error_stack is NULL
 */
LIBRARY_API int udaAddErrorToStack(UDA_ERROR_STACK* error_stack, int type, const char* location, int code,
                                   const char* msg);

/**
 * Append all records of source to error_stack, in order, respecting UDA_MAX_ERRORS.
 *
 * @return 0 on success, 1 if some records were dropped, -1 if either argument is NULL
 */
LIBRARY_API int udaAppendErrorStack(UDA_ERROR_STACK* error_stack, const UDA_ERROR_STACK* source);

/**
 * If a request failed (code != 0) but error_stack holds no records, append a fallback record carrying the
 * original code, so that the response header and the error state cannot disagree.
 *
 * @return 1 if a fallback record was added, 0 otherwise
 */
LIBRARY_API int udaEnsureErrorRecorded(UDA_ERROR_STACK* error_stack, int code, const char* location);

/**
 * Move the records added to this library's global error stack since it held `start` records (see udaNumErrors())
 * onto error_stack, removing them from the global stack so they are reported exactly once. Used to capture errors
 * from helpers that still record into global state. error_stack may be NULL to discard them.
 *
 * @return 0 on success, 1 if some records were dropped because error_stack was full
 */
LIBRARY_API int udaMoveErrorsSince(int start, UDA_ERROR_STACK* error_stack);

LIBRARY_API void freeIdamErrorStack(UDA_ERROR_STACK* errorstack);
LIBRARY_API void closeUdaError(void);

#ifdef __cplusplus
}
#endif

#define UDA_ADD_ERROR(ERR, MSG) addIdamError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG) addIdamError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_CLIENTSERVER_ERRORLOG_H
