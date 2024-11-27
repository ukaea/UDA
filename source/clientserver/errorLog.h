#include "udaStructs.h"
#include <time.h>

namespace uda::client_server
{

enum class ErrorType : int {
    None = 0,
    System = 1,
    Code = 2,
    Plugin = 3,
};

std::string format_as(ErrorType error_type);

void error_log(ClientBlock client_block, RequestBlock request_block, ErrorStack* error_stack);

void init_error_stack(void);

void init_error_records(const ErrorStack* errorstack);

void print_error_stack(void);

void add_error(ErrorType type, const char* location, int code, const char* msg);

UdaError create_error(ErrorType type, const char* location, int code, const char* msg);

void concat_error(ErrorStack* errorstackout);

void free_error_stack(ErrorStack* errorstack);

void close_error(void);

} // namespace uda::client_server

#define UDA_ADD_ERROR(ERR, MSG) add_error(ErrorType::Code, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(MSG) add_error(ErrorType::System, __func__, errno, MSG)
#define UDA_THROW_ERROR(ERR, MSG)                                                                                      \
    add_error(ErrorType::Code, __func__, ERR, MSG);                                                                \
    return ERR;
