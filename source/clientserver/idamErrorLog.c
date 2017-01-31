/*---------------------------------------------------------------
* Server Error Log Utilities
*
* Log Format:	client userid, date, client request,
*		error class, error code, error message
*
*--------------------------------------------------------------*/

#include "idamErrorLog.h"

#include <logging/idamLog.h>
#include <stdlib.h>
#include <include/idamclientserverprivate.h>
#include "stringUtils.h"

IDAMERRORSTACK idamerrorstack;

void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK errorstack)
{
    unsigned int i;

    time_t calendar;            // Simple Calendar Date & Time
    struct tm* broken;            // Broken Down calendar Time
    static char accessdate[DATELENGTH];    // The Calendar Time as a formatted String

    if (errorstack.nerrors == 0) return;

// Client's Userid: from the client_block structure

// Calendar Time

    time(&calendar);
    broken = gmtime(&calendar);
    asctime_r(broken, accessdate);

    convertNonPrintable2(accessdate);
    TrimString(accessdate);

// Client Request: From the request_block structure

// Error Codes & Error Message: from Error Message Stack

// Write the Log Record & Flush the fd

    idamLog(LOG_ERROR, "0 %s [%s] [%d %s %d %d %s %s %s %s %s %s %s]\n",
            client_block.uid, accessdate, request.request, request.signal, request.exp_number,
            request.pass, request.tpass, request.path, request.file, request.format, request.archive,
            request.device_name, request.server);

    for (i = 0; i < errorstack.nerrors; i++) {
        idamLog(LOG_ERROR, "1 %s [%s] %d %d [%s] [%s]\n", client_block.uid, accessdate, errorstack.idamerror[i].type,
                errorstack.idamerror[i].code, errorstack.idamerror[i].location, errorstack.idamerror[i].msg);
    }
}

// Initialise the Error Stack

void initIdamErrorStack(IDAMERRORSTACK* errorstack)
{
    errorstack->nerrors = 0;
    errorstack->idamerror = NULL;
}

void initIdamErrorRecords(IDAMERRORSTACK* errorstack)
{
    unsigned int i;
    for (i = 0; i < errorstack->nerrors; i++) {
        errorstack->idamerror[i].type = 0;
        errorstack->idamerror[i].code = 0;
        errorstack->idamerror[i].location[0] = '\0';
        errorstack->idamerror[i].msg[0] = '\0';
    }
}

void printIdamErrorStack(IDAMERRORSTACK errorstack)
{
    unsigned int i;
    if (errorstack.nerrors == 0) {
        idamLog(LOG_DEBUG, "Empty Error Stack\n");
        return;
    }
    for (i = 0; i < errorstack.nerrors; i++) {
        idamLog(LOG_DEBUG, "%d %d %d %s %s\n", i, errorstack.idamerror[i].type,
                errorstack.idamerror[i].code, errorstack.idamerror[i].location, errorstack.idamerror[i].msg);
    }
}

// Add an Error to the Stack
//
// Error Classes: 	0 => System Error (i.e. a Non Zero errno)
//			1 => Code Error
//			2 => Plugin Error

void addIdamError(IDAMERRORSTACK* errorstack, int type, const char* location, int code, const char* msg)
{
    errorstack->idamerror = (IDAMERROR*) realloc((void*) errorstack->idamerror,
                                                 ((errorstack->nerrors + 1)) * sizeof(IDAMERROR));
    errorstack->idamerror[errorstack->nerrors].type = type;
    errorstack->idamerror[errorstack->nerrors].code = code;

    strncpy(errorstack->idamerror[errorstack->nerrors].location, location, STRING_LENGTH - 1);
    errorstack->idamerror[errorstack->nerrors].location[STRING_LENGTH - 1] = '\0';

    strncpy(errorstack->idamerror[errorstack->nerrors].msg, msg, STRING_LENGTH - 1);
    errorstack->idamerror[errorstack->nerrors].msg[STRING_LENGTH - 1] = '\0';
    int lmsg0 = (int) strlen(errorstack->idamerror[errorstack->nerrors].msg);

    if (type == SYSTEMERRORTYPE) {
        char* errmsg = strerror(code);
        int lmsg1 = (int) strlen(errmsg);
        if (lmsg0 == 0) {
            strncpy(errorstack->idamerror[errorstack->nerrors].msg, errmsg, STRING_LENGTH - 1);
            errorstack->idamerror[errorstack->nerrors].msg[STRING_LENGTH - 1] = '\0';
        } else {
            if ((lmsg0 + 2) < STRING_LENGTH) {
                strcat(errorstack->idamerror[errorstack->nerrors].msg, "; ");
                if ((lmsg0 + lmsg1 + 2) < STRING_LENGTH) {
                    strcat(errorstack->idamerror[errorstack->nerrors].msg, errmsg);
                } else {
                    strncat(errorstack->idamerror[errorstack->nerrors].msg, errmsg,
                            ((unsigned int) (STRING_LENGTH - 1 - (lmsg0 + 2))));
                    errorstack->idamerror[errorstack->nerrors].msg[STRING_LENGTH - 1] = '\0';
                }
            }
        }
    }

    (errorstack->nerrors)++;
}

//extern IDAMERRORSTACK *getIdamServerPluginErrorStack(){ return &errorstack; }

// Concatenate Error Stack structures

void concatIdamError(IDAMERRORSTACK errorstackin, IDAMERRORSTACK* errorstackout)
{
    unsigned int i;
    unsigned int iold;
    unsigned int inew;

    if (errorstackin.nerrors <= 0) return;

    iold = errorstackout->nerrors;
    inew = errorstackin.nerrors + errorstackout->nerrors;

    errorstackout->idamerror = (IDAMERROR*) realloc((void*) errorstackout->idamerror,
                                                    ((unsigned int) inew * sizeof(IDAMERROR)));

    for (i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = errorstackin.idamerror[i - iold];
    }
    errorstackout->nerrors = inew;
}

// Free Stack Heap

void closeIdamError(IDAMERRORSTACK* errorstack)
{
    if (errorstack->idamerror != NULL) free((void*) errorstack->idamerror);
    initIdamErrorStack(errorstack);
}
