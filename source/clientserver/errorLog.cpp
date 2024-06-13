#include "errorLog.h"

#include <cstdlib>
#include <vector>

#include "common/stringUtils.h"
#include "logging/logging.h"
#include "uda/client.h"

using namespace uda::logging;

static std::vector<uda::client_server::UdaError> udaerrorstack;

int udaNumErrors(void)
{
    return static_cast<int>(udaerrorstack.size());
}

const char* udaGetErrorMessage(int err_num)
{
    if (err_num > (int)udaerrorstack.size() && err_num < (int)udaerrorstack.size()) {
        return udaerrorstack[err_num].msg;
    }
    return "no error found";
}

const char* udaGetErrorLocation(int err_num)
{
    if (err_num > (int)udaerrorstack.size() && err_num < (int)udaerrorstack.size()) {
        return udaerrorstack[err_num].location;
    }
    return "no error found";
}

void uda::client_server::error_log(ClientBlock client_block, RequestBlock request_block, ErrorStack* error_stack)
{
    UdaError* errors = nullptr;
    unsigned int nerrors;

    if (error_stack == nullptr) {
        errors = udaerrorstack.data();
        nerrors = udaerrorstack.size();
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

    static char access_date[UDA_DATE_LENGTH]; // The Calendar Time as a formatted String

#ifndef _WIN32
    asctime_r(broken, access_date);
#else
    asctime_s(accessdate, UDA_DATE_LENGTH, broken);
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
            "1 {} [{}] {} {} [{}] [{}]", client_block.uid, access_date, errors[i].type,
            errors[i].code, errors[i].location, errors[i].msg);
    }
}

// Initialise the Error Stack

void uda::client_server::init_error_stack()
{
    udaerrorstack.clear();
}

void uda::client_server::init_error_records(const ErrorStack* errorstack)
{
    for (unsigned int i = 0; i < errorstack->nerrors; i++) {
        errorstack->idamerror[i].type = 0;
        errorstack->idamerror[i].code = 0;
        errorstack->idamerror[i].location[0] = '\0';
        errorstack->idamerror[i].msg[0] = '\0';
    }
}

void uda::client_server::print_error_stack()
{
    if (udaerrorstack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Empty Error Stack");
        return;
    }
    int i = 1;
    for (const auto& error : udaerrorstack) {
        UDA_LOG(UDA_LOG_DEBUG, "{} {} {} {} {}", i, error.type, error.code, error.location, error.msg);
        ++i;
    }
}

// Add an Error to the Stack
//
// Error Classes:     0 => System Error (i.e. a Non Zero errno)
//            1 => Code Error
//            2 => Plugin Error

uda::client_server::UdaError uda::client_server::create_error(int type, const char* location, int code, const char* msg)
{
    UdaError error;

    error.type = type;
    error.code = code;
    copy_string(location, error.location, STRING_LENGTH);
    copy_string(msg, error.msg, STRING_LENGTH);

    size_t lmsg0 = strlen(error.msg);

    if (type == UDA_SYSTEM_ERROR_TYPE) {
        const char* errmsg = strerror(code);
        size_t lmsg1 = strlen(errmsg);
        if (lmsg0 == 0) {
            copy_string(errmsg, error.msg, STRING_LENGTH);
        } else {
            if ((lmsg0 + 2) < STRING_LENGTH) {
                strcat(error.msg, "; ");
                if ((lmsg0 + lmsg1 + 2) < STRING_LENGTH) {
                    strcat(error.msg, errmsg);
                } else {
                    strncat(error.msg, errmsg, ((unsigned int)(STRING_LENGTH - 1 - (lmsg0 + 2))));
                    error.msg[STRING_LENGTH - 1] = '\0';
                }
            }
        }
    }

    return error;
}

void uda::client_server::add_error(int type, const char* location, int code, const char* msg)
{
    udaerrorstack.push_back(create_error(type, location, code, msg));
}

// Concatenate Error Stack structures

void uda::client_server::concat_error(ErrorStack* errorstackout)
{
    if (udaerrorstack.empty()) {
        return;
    }

    unsigned int iold = errorstackout->nerrors;
    unsigned int inew = udaerrorstack.size() + errorstackout->nerrors;

    errorstackout->idamerror = (UdaError*)realloc((void*)errorstackout->idamerror, (inew * sizeof(UdaError)));

    for (unsigned int i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = udaerrorstack[i - iold];
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
