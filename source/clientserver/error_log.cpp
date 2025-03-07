#include "error_log.h"

#include <cstdlib>
#include <vector>

#include "common/string_utils.h"
#include "logging/logging.h"
#include "uda/client.h"

using namespace uda::logging;
using namespace uda::common;

const char* uda::client_server::format_as(ErrorType error_type)
{
    switch (error_type) {
        case ErrorType::None:
            return "ErrorType::None";
        case ErrorType::System:
            return "ErrorType::System";
        case ErrorType::Code:
            return "ErrorType::Code";
        case ErrorType::Plugin:
            return "ErrorType::Plugin";
    }
    return "";
}

void uda::client_server::print_errors(const std::vector<UdaError>& error_stack, ClientBlock client_block, RequestBlock request_block)
{
    if (error_stack.empty()) {
        return;
    }

    time_t calendar;
    time(&calendar);

    tm* broken = gmtime(&calendar);

    constexpr size_t access_date_length = 27;
    static char access_date[access_date_length]; // The Calendar Time as a formatted String

#ifndef _WIN32
    asctime_r(broken, access_date);
#else
    asctime_s(accessdate, access_date_length, broken);
#endif

    convert_non_printable2(access_date);
    trim_string(access_date);

    for (size_t i = 0; i < request_block.size(); ++i) {
        auto* request = &request_block[i];
        log(UDA_LOG_ERROR, __FILE__, __LINE__,
            "0 {} [{}] [{} {} {} {} {} {} {} {} {} {} {}]", client_block.uid, access_date,
            request->request, request->signal, request->exp_number, request->pass, request->tpass, request->path,
            request->file, request->format, request->archive, request->device_name, request->server);
    }

    for (const auto& error : error_stack) {
        log(UDA_LOG_ERROR, __FILE__, __LINE__,
            "1 {} [{}] {} {} [{}] [{}]", client_block.uid, access_date, format_as(error.type),
            error.code, error.location, error.msg);
    }
}

// Initialise the Error Stack

void uda::client_server::print_error_stack(const std::vector<UdaError>& error_stack)
{
    if (error_stack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Empty Error Stack");
        return;
    }
    int i = 1;
    for (const auto& [type, code, location, msg] : error_stack) {
        UDA_LOG(UDA_LOG_DEBUG, "{} {} {} {} {}", i, format_as(type), code, location, msg);
        ++i;
    }
}

// Add an Error to the Stack
//
// Error Classes:     0 => System Error (i.e. a Non Zero errno)
//            1 => Code Error
//            2 => Plugin Error

uda::client_server::UdaError uda::client_server::create_error(const ErrorType type, const char* location, const int code, const char* msg)
{
    UdaError error;

    error.type = type;
    error.code = code;
    copy_string(location, error.location, StringLength);
    copy_string(msg, error.msg, StringLength);

    size_t lmsg0 = strlen(error.msg);

    if (type == ErrorType::System) {
        const char* errmsg = strerror(code);
        size_t lmsg1 = strlen(errmsg);
        if (lmsg0 == 0) {
            copy_string(errmsg, error.msg, StringLength);
        } else {
            if ((lmsg0 + 2) < StringLength) {
                strcat(error.msg, "; ");
                if ((lmsg0 + lmsg1 + 2) < StringLength) {
                    strcat(error.msg, errmsg);
                } else {
                    strncat(error.msg, errmsg, ((unsigned int)(StringLength - 1 - (lmsg0 + 2))));
                    error.msg[StringLength - 1] = '\0';
                }
            }
        }
    }

    return error;
}
