/*---------------------------------------------------------------
* Server Error Log Utilities
*
* Log Format:	client userid, date, client request,
*		error class, error code, error message
*
*--------------------------------------------------------------*/

#include "errorLog.h"

#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>

static IDAMERRORSTACK udaerrorstack;

int udaNumErrors()
{
    return udaerrorstack.nerrors;
}

void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK* errorstack)
{
    if (errorstack == NULL) {
        errorstack = &udaerrorstack;
    }

    if (errorstack->nerrors == 0) {
        return;
    }

    time_t calendar;
    time(&calendar);

    struct tm* broken = gmtime(&calendar);

    static char accessdate[DATELENGTH];     // The Calendar Time as a formatted String
    asctime_r(broken, accessdate);

    convertNonPrintable2(accessdate);
    TrimString(accessdate);

    idamLog(UDA_LOG_ERROR, "0 %s [%s] [%d %s %d %d %s %s %s %s %s %s %s]\n",
            client_block.uid, accessdate, request.request, request.signal, request.exp_number,
            request.pass, request.tpass, request.path, request.file, request.format, request.archive,
            request.device_name, request.server);

    int i;
    for (i = 0; i < errorstack->nerrors; i++) {
        idamLog(UDA_LOG_ERROR, "1 %s [%s] %d %d [%s] [%s]\n", client_block.uid, accessdate,
                errorstack->idamerror[i].type,
                errorstack->idamerror[i].code, errorstack->idamerror[i].location, errorstack->idamerror[i].msg);
    }
}

// Initialise the Error Stack

void initIdamErrorStack()
{
    udaerrorstack.nerrors = 0;
    udaerrorstack.idamerror = NULL;
}

void initIdamErrorRecords()
{
    unsigned int i;
    for (i = 0; i < udaerrorstack.nerrors; i++) {
        udaerrorstack.idamerror[i].type = 0;
        udaerrorstack.idamerror[i].code = 0;
        udaerrorstack.idamerror[i].location[0] = '\0';
        udaerrorstack.idamerror[i].msg[0] = '\0';
    }
}

void printIdamErrorStack()
{
    unsigned int i;
    if (udaerrorstack.nerrors == 0) {
        IDAM_LOG(UDA_LOG_DEBUG, "Empty Error Stack\n");
        return;
    }
    for (i = 0; i < udaerrorstack.nerrors; i++) {
        IDAM_LOGF(UDA_LOG_DEBUG, "%d %d %d %s %s\n", i, udaerrorstack.idamerror[i].type,
                  udaerrorstack.idamerror[i].code, udaerrorstack.idamerror[i].location, udaerrorstack.idamerror[i].msg);
    }
}

// Add an Error to the Stack
//
// Error Classes: 	0 => System Error (i.e. a Non Zero errno)
//			1 => Code Error
//			2 => Plugin Error

void addIdamError(int type, const char* location, int code, const char* msg)
{
    udaerrorstack.idamerror = (IDAMERROR*)realloc((void*)udaerrorstack.idamerror,
                                                  ((udaerrorstack.nerrors + 1)) * sizeof(IDAMERROR));
    udaerrorstack.idamerror[udaerrorstack.nerrors].type = type;
    udaerrorstack.idamerror[udaerrorstack.nerrors].code = code;

    strncpy(udaerrorstack.idamerror[udaerrorstack.nerrors].location, location, STRING_LENGTH - 1);
    udaerrorstack.idamerror[udaerrorstack.nerrors].location[STRING_LENGTH - 1] = '\0';

    strncpy(udaerrorstack.idamerror[udaerrorstack.nerrors].msg, msg, STRING_LENGTH - 1);
    udaerrorstack.idamerror[udaerrorstack.nerrors].msg[STRING_LENGTH - 1] = '\0';
    int lmsg0 = (int)strlen(udaerrorstack.idamerror[udaerrorstack.nerrors].msg);

    if (type == SYSTEMERRORTYPE) {
        char* errmsg = strerror(code);
        int lmsg1 = (int)strlen(errmsg);
        if (lmsg0 == 0) {
            strncpy(udaerrorstack.idamerror[udaerrorstack.nerrors].msg, errmsg, STRING_LENGTH - 1);
            udaerrorstack.idamerror[udaerrorstack.nerrors].msg[STRING_LENGTH - 1] = '\0';
        } else {
            if ((lmsg0 + 2) < STRING_LENGTH) {
                strcat(udaerrorstack.idamerror[udaerrorstack.nerrors].msg, "; ");
                if ((lmsg0 + lmsg1 + 2) < STRING_LENGTH) {
                    strcat(udaerrorstack.idamerror[udaerrorstack.nerrors].msg, errmsg);
                } else {
                    strncat(udaerrorstack.idamerror[udaerrorstack.nerrors].msg, errmsg,
                            ((unsigned int)(STRING_LENGTH - 1 - (lmsg0 + 2))));
                    udaerrorstack.idamerror[udaerrorstack.nerrors].msg[STRING_LENGTH - 1] = '\0';
                }
            }
        }
    }

    (udaerrorstack.nerrors)++;
}

// Concatenate Error Stack structures

void concatIdamError(IDAMERRORSTACK* errorstackout)
{
    unsigned int i;
    unsigned int iold;
    unsigned int inew;

    if (udaerrorstack.nerrors <= 0) {
        return;
    }

    iold = errorstackout->nerrors;
    inew = udaerrorstack.nerrors + errorstackout->nerrors;

    errorstackout->idamerror = (IDAMERROR*)realloc((void*)errorstackout->idamerror, (inew * sizeof(IDAMERROR)));

    for (i = iold; i < inew; i++) {
        errorstackout->idamerror[i] = udaerrorstack.idamerror[i - iold];
    }
    errorstackout->nerrors = inew;
}

void freeIdamErrorStack(IDAMERRORSTACK* errorstack)
{
    free(errorstack->idamerror);
    errorstack->nerrors = 0;
    errorstack->idamerror = NULL;
}

// Free Stack Heap

void closeIdamError()
{
    free(udaerrorstack.idamerror);
    initIdamErrorStack();
}
