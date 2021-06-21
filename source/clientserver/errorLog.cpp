/*---------------------------------------------------------------
* Server Error Log Utilities
*
* Log Format:	client userid, date, client request,
*		error class, error code, error message
*
*--------------------------------------------------------------*/

#include "errorLog.h"

#include <cstdlib>
#include <vector>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>

static std::vector<UDA_ERROR> udaerrorstack;

int udaNumErrors()
{
    return udaerrorstack.size();
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

    static char accessdate[DATELENGTH];     // The Calendar Time as a formatted String
    
#ifndef _WIN32
    asctime_r(broken, accessdate);
#else
    asctime_s(accessdate, DATELENGTH, broken);
#endif

    convertNonPrintable2(accessdate);
    TrimString(accessdate);

    for (int i = 0; i < request_block.num_requests; ++i) {
        auto request = &request_block.requests[0];
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

void initIdamErrorStack()
{
    udaerrorstack.clear();
}

void initErrorRecords(const UDA_ERROR_STACK* errorstack)
{
    for (unsigned int i = 0; i < errorstack->nerrors; i++) {
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
// Error Classes: 	0 => System Error (i.e. a Non Zero errno)
//			1 => Code Error
//			2 => Plugin Error

void addIdamError(int type, const char* location, int code, const char* msg)
{
    UDA_ERROR error;

    error.type = type;
    error.code = code;
    strncpy(error.location, location, STRING_LENGTH - 1);
    error.location[STRING_LENGTH - 1] = '\0';
    strncpy(error.msg, msg, STRING_LENGTH - 1);
    error.msg[STRING_LENGTH - 1] = '\0';

    size_t lmsg0 = strlen(error.msg);

    if (type == SYSTEMERRORTYPE) {
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

    udaerrorstack.push_back(error);
}

// Concatenate Error Stack structures

void concatUdaError(UDA_ERROR_STACK* errorstackout)
{
    if (udaerrorstack.empty()) {
        return;
    }

    unsigned int iold = errorstackout->nerrors;
    unsigned int inew = udaerrorstack.size() + errorstackout->nerrors;

    errorstackout->idamerror = (UDA_ERROR*)realloc((void*)errorstackout->idamerror, (inew * sizeof(UDA_ERROR)));

    for (unsigned int i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = udaerrorstack[i - iold];
    }
    errorstackout->nerrors = inew;
}

void freeIdamErrorStack(UDA_ERROR_STACK* errorstack)
{
    // "FIX" : this is causing segfaults when using multiple clients (eg. get and put) 
    //         apparently due to both trying to free the same memory. Needs fixing properly.
    //    free(errorstack->idamerror);
      
    errorstack->nerrors = 0;
    errorstack->idamerror = nullptr;
}

// Free Stack Heap

void closeUdaError()
{
    initIdamErrorStack();
}
