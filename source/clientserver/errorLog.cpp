#include "errorLog.h"

#include <cstdlib>
#include <vector>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>

static std::vector<UDA_ERROR> udaerrorstack;

int udaNumErrors()
{
    return static_cast<int>(udaerrorstack.size());
}

void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack)
{
    UDA_ERROR* errors = nullptr;
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

    static char accessdate[UDA_DATE_LENGTH];     // The Calendar Time as a formatted String
    
#ifndef _WIN32
    asctime_r(broken, accessdate);
#else
    asctime_s(accessdate, UDA_DATE_LENGTH, broken);
#endif

    convertNonPrintable2(accessdate);
    TrimString(accessdate);

    for (int i = 0; i < request_block.num_requests; ++i) {
        auto request = &request_block.requests[i];
        udaLog(UDA_LOG_ERROR, "0 %s [%s] [%d %s %d %d %s %s %s %s %s %s %s]\n",
               client_block.uid, accessdate, request->request, request->signal, request->exp_number,
               request->pass, request->tpass, request->path, request->file, request->format, request->archive,
               request->device_name, request->server);
    }

    for (unsigned int i = 0; i < nerrors; i++) {
        udaLog(UDA_LOG_ERROR, "1 %s [%s] %d %d [%s] [%s]\n", client_block.uid, accessdate,
                errors[i].type, errors[i].code, errors[i].location, errors[i].msg);
    }
}

// Initialise the Error Stack

void initUdaErrorStack()
{
    udaerrorstack.clear();
}

void initErrorRecords(UDA_ERROR_STACK* errorstack)
{
    for (unsigned int i = 0; i < UDA_MAX_ERRORS; i++) {
        errorstack->idamerror[i].type = 0;
        errorstack->idamerror[i].code = 0;
        errorstack->idamerror[i].location[0] = '\0';
        errorstack->idamerror[i].msg[0] = '\0';
    }
}

void printIdamErrorStack()
{
    if (udaerrorstack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Empty Error Stack\n");
        return;
    }
    int i = 1;
    for (const auto& error : udaerrorstack) {
        UDA_LOG(UDA_LOG_DEBUG, "%d %d %d %s %s\n", i, error.type, error.code, error.location, error.msg);
        ++i;
    }
}

// Add an Error to the Stack
//
// Error Classes:     0 => System Error (i.e. a Non Zero errno)
//            1 => Code Error
//            2 => Plugin Error

namespace {

// Build a single error record. Strings are copied, truncated and terminated; null strings are treated as empty.
// System errors have the strerror() text for their code appended to the message.
UDA_ERROR make_error(int type, const char* location, int code, const char* msg)
{
    UDA_ERROR error = {};

    error.type = type;
    error.code = code;
    strncpy(error.location, location != nullptr ? location : "", STRING_LENGTH - 1);
    error.location[STRING_LENGTH - 1] = '\0';
    strncpy(error.msg, msg != nullptr ? msg : "", STRING_LENGTH - 1);
    error.msg[STRING_LENGTH - 1] = '\0';

    size_t lmsg0 = strlen(error.msg);

    if (type == UDA_SYSTEM_ERROR_TYPE) {
        const char* errmsg = strerror(code);
        size_t lmsg1 = strlen(errmsg);
        if (lmsg0 == 0) {
            strncpy(error.msg, errmsg, STRING_LENGTH - 1);
            error.msg[STRING_LENGTH - 1] = '\0';
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

} // namespace

void addIdamError(int type, const char* location, int code, const char* msg)
{
    udaerrorstack.push_back(make_error(type, location, code, msg));
}

int udaAddErrorToStack(UDA_ERROR_STACK* error_stack, int type, const char* location, int code, const char* msg)
{
    if (error_stack == nullptr) {
        return -1;
    }
    if (error_stack->nerrors >= UDA_MAX_ERRORS) {
        // Keep the earliest records: the first record is the root cause and drives the response header
        return 1;
    }
    error_stack->idamerror[error_stack->nerrors] = make_error(type, location, code, msg);
    error_stack->nerrors++;
    return 0;
}

int udaAppendErrorStack(UDA_ERROR_STACK* error_stack, const UDA_ERROR_STACK* source)
{
    if (error_stack == nullptr || source == nullptr) {
        return -1;
    }
    int rc = 0;
    unsigned int count = source->nerrors < UDA_MAX_ERRORS ? source->nerrors : UDA_MAX_ERRORS;
    for (unsigned int i = 0; i < count; ++i) {
        if (error_stack->nerrors >= UDA_MAX_ERRORS) {
            rc = 1;
            break;
        }
        error_stack->idamerror[error_stack->nerrors++] = source->idamerror[i];
    }
    return rc;
}

int udaEnsureErrorRecorded(UDA_ERROR_STACK* error_stack, int code, const char* location)
{
    if (error_stack == nullptr || code == 0 || error_stack->nerrors > 0) {
        return 0;
    }
    char msg[STRING_LENGTH];
    snprintf(msg, STRING_LENGTH, "Request failed with error code %d but no error details were reported", code);
    udaAddErrorToStack(error_stack, UDA_CODE_ERROR_TYPE, location, code, msg);
    return 1;
}

int udaMoveErrorsSince(int start, UDA_ERROR_STACK* error_stack)
{
    if (start < 0) {
        start = 0;
    }
    if (static_cast<size_t>(start) >= udaerrorstack.size()) {
        return 0;
    }
    int rc = 0;
    if (error_stack != nullptr) {
        for (size_t i = start; i < udaerrorstack.size(); ++i) {
            if (error_stack->nerrors >= UDA_MAX_ERRORS) {
                rc = 1;
                break;
            }
            error_stack->idamerror[error_stack->nerrors++] = udaerrorstack[i];
        }
    }
    udaerrorstack.resize(start);
    return rc;
}

// Concatenate Error Stack structures

void concatUdaError(UDA_ERROR_STACK* errorstackout)
{
    if (udaerrorstack.empty()) {
        return;
    }

    unsigned int iold = errorstackout->nerrors;
    unsigned int inew = udaerrorstack.size() + errorstackout->nerrors;
	if (inew > UDA_MAX_ERRORS) {
		inew = UDA_MAX_ERRORS;
	}

    for (unsigned int i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = udaerrorstack[i - iold];
    }
    errorstackout->nerrors = inew;
}

void freeIdamErrorStack(UDA_ERROR_STACK* errorstack)
{
    // "FIX" : this is causing segfaults when using multiple clients (eg. get and put) 
    //         apparently due to both trying to free the same memory. Needs fixing properly.
      
    errorstack->nerrors = 0;
}

// Free Stack Heap

void closeUdaError()
{
    initUdaErrorStack();
}
