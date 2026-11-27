#include "errorLog.h"

#include <cstdlib>
#include <vector>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>

#ifndef _WIN32
#  include <sys/time.h>
#endif

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


void addIdamError(int type, const char* location, int code, const char* msg)
{
    udaerrorstack.push_back(udaMakeError(type, location, code, msg));
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

// Diagnostics context forwarding to this library's own error stack and logger

namespace {

int global_error_sink(void*, int type, const char* location, int code, const char* message)
{
    addIdamError(type, location, code, message);
    return 0;
}

void global_log_sink(void*, int level, const char* file, int line, const char* message)
{
    // Same layout as UDA_LOG
#ifndef _WIN32
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm tm_now = {};
    localtime_r(&tv.tv_sec, &tm_now);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y:%m:%dT%H:%M:%S", &tm_now);
    udaLog((LOG_LEVEL)level, "%s.%dZ, %s:%d >> %s", timestamp, (int)tv.tv_usec, file != nullptr ? file : "", line,
           message);
#else
    udaLog((LOG_LEVEL)level, "%s:%d >> %s", file != nullptr ? file : "", line, message);
#endif
}

} // namespace

UDA_DIAGNOSTICS udaGlobalDiagnostics()
{
    UDA_DIAGNOSTICS diag = {};
    diag.error_function = global_error_sink;
    diag.log_function = global_log_sink;
    diag.log_level = (int)udaGetLogLevel();
    return diag;
}
