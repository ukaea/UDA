#include "udaStructs.h"
#include <time.h>

#define UDA_DATE_LENGTH 27

//--------------------------------------------------------
// Error Management

#define UDA_SYSTEM_ERROR_TYPE 1
#define UDA_CODE_ERROR_TYPE 2
#define UDA_PLUGIN_ERROR_TYPE 3

void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack);
void initErrorStack(void);
void initErrorRecords(const UDA_ERROR_STACK* errorstack);
void udaPrintErrorStack(void);
void udaAddError(int type, const char* location, int code, const char* msg);
UDA_ERROR udaCreateError(int type, const char* location, int code, const char* msg);
void udaConcatError(UDA_ERROR_STACK* errorstackout);
void udaFreeErrorStack(UDA_ERROR_STACK* errorstack);
void udaCloseError(void);

#define UDA_ADD_ERROR(ERR, MSG) udaAddError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) udaAddError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG)                                                                                      \
    udaAddError(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG);                                                             \
    return ERR;
