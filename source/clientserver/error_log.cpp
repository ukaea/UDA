#include "error_log.h"

#include <cstdlib>
#include <vector>

#include "common/string_utils.h"
#include "logging/logging.h"
#include "uda/client.h"

using namespace uda::logging;
using namespace uda::common;

static std::vector<uda::client_server::UdaError> uda_error_stack;

int udaNumErrors()
{
    return static_cast<int>(uda_error_stack.size());
}

const char* udaGetErrorMessage(int err_num)
{
    if (err_num > (int)uda_error_stack.size() && err_num < (int)uda_error_stack.size()) {
        return uda_error_stack[err_num].msg;
    }
    return "no error found";
}

const char* udaGetErrorLocation(int err_num)
{
    if (err_num > (int)uda_error_stack.size() && err_num < (int)uda_error_stack.size()) {
        return uda_error_stack[err_num].location;
    }
    return "no error found";
}

std::string uda::client_server::format_as(ErrorType error_type)
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

void uda::client_server::error_log(ClientBlock client_block, RequestBlock request_block, ErrorStack* error_stack)
{
    UdaError* errors = nullptr;
    unsigned int nerrors;

    if (error_stack == nullptr) {
        errors = uda_error_stack.data();
        nerrors = uda_error_stack.size();
    } else {
        errors = error_stack->idamerror;
        nerrors = error_stack->nerrors;
    }

    if (nerrors == 0) {
        return;
    }

    time_t calendar;
    time(&calendar);

    struct tm* broken = gmtime(&calendar);

    constexpr size_t access_date_length = 27;
    static char access_date[access_date_length]; // The Calendar Time as a formatted String

#ifndef _WIN32
    asctime_r(broken, access_date);
#else
    asctime_s(accessdate, access_date_length, broken);
#endif

    convert_non_printable2(access_date);
    trim_string(access_date);

    for (int i = 0; i < request_block.num_requests; ++i) {
        auto request = &request_block.requests[i];
        log(UDA_LOG_ERROR, __FILE__, __LINE__,
            "0 {} [{}] [{} {} {} {} {} {} {} {} {} {} {}]", client_block.uid, access_date,
            request->request, request->signal, request->exp_number, request->pass, request->tpass, request->path,
            request->file, request->format, request->archive, request->device_name, request->server);
    }

    for (unsigned int i = 0; i < nerrors; i++) {
        log(UDA_LOG_ERROR, __FILE__, __LINE__,
            "1 {} [{}] {} {} [{}] [{}]", client_block.uid, access_date, format_as(errors[i].type),
            errors[i].code, errors[i].location, errors[i].msg);
    }
}

// Initialise the Error Stack

void uda::client_server::init_error_stack()
{
    uda_error_stack.clear();
}

void uda::client_server::init_error_records(const ErrorStack* errorstack)
{
    for (unsigned int i = 0; i < errorstack->nerrors; i++) {
        errorstack->idamerror[i].type = ErrorType::None;
        errorstack->idamerror[i].code = 0;
        errorstack->idamerror[i].location[0] = '\0';
        errorstack->idamerror[i].msg[0] = '\0';
    }
}

void uda::client_server::print_error_stack()
{
    if (uda_error_stack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Empty Error Stack");
        return;
    }
    int i = 1;
    for (const auto& error : uda_error_stack) {
        UDA_LOG(UDA_LOG_DEBUG, "{} {} {} {} {}", i, format_as(error.type), error.code, error.location, error.msg);
        ++i;
    }
}

// Add an Error to the Stack
//
// Error Classes:     0 => System Error (i.e. a Non Zero errno)
//            1 => Code Error
//            2 => Plugin Error

uda::client_server::UdaError uda::client_server::create_error(ErrorType type, const char* location, int code, const char* msg)
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

void uda::client_server::add_error(ErrorType type, const char* location, int code, const char* msg)
{
    uda_error_stack.push_back(create_error(type, location, code, msg));
}

// Concatenate Error Stack structures

void uda::client_server::concat_error(ErrorStack* errorstackout)
{
    if (uda_error_stack.empty()) {
        return;
    }

    unsigned int iold = errorstackout->nerrors;
    unsigned int inew = uda_error_stack.size() + errorstackout->nerrors;

    errorstackout->idamerror = (UdaError*)realloc((void*)errorstackout->idamerror, (inew * sizeof(UdaError)));

    for (unsigned int i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = uda_error_stack[i - iold];
    }
    errorstackout->nerrors = inew;
}

void uda::client_server::free_error_stack(ErrorStack* errorstack)
{
    // "FIX" : this is causing segfaults when using multiple clients (eg. get and put)
    //         apparently due to both trying to free the same memory. Needs fixing properly.
    //    free(errorstack->idamerror);

    errorstack->nerrors = 0;
    errorstack->idamerror = nullptr;
}

// Free Stack Heap

void uda::client_server::close_error()
{
    init_error_stack();
}
