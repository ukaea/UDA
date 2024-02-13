#include "udaStructs.h"
#include <time.h>

#define UDA_DATE_LENGTH 27

//--------------------------------------------------------
// Error Management

#define UDA_SYSTEM_ERROR_TYPE 1
#define UDA_CODE_ERROR_TYPE 2
#define UDA_PLUGIN_ERROR_TYPE 3

namespace uda::client_server
{

void error_log(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, ErrorStack* error_stack);

void init_error_stack(void);

void init_error_records(const ErrorStack* errorstack);

void print_error_stack(void);

void add_error(int type, const char* location, int code, const char* msg);

UDA_ERROR create_error(int type, const char* location, int code, const char* msg);

void concat_error(ErrorStack* errorstackout);

void free_error_stack(ErrorStack* errorstack);

void close_error(void);

} // namespace uda::client_server

#define UDA_ADD_ERROR(ERR, MSG) add_error(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) add_error(UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG)                                                                                      \
    add_error(UDA_CODE_ERROR_TYPE, __func__, ERR, MSG);                                                                \
    return ERR;
