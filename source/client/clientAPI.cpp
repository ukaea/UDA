/*---------------------------------------------------------------
* Reads the Requested Data
*
* Input Arguments:	1) IDA File Name
*			2) IDA Signal Name
*			3) IDA Pass Number (-1 => LATEST)
*			4) Plasma Pulse Number
*
* Returns:
*
*--------------------------------------------------------------*/
#include "clientAPI.h"

#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  include <string.h>
#  define strcasecmp _stricmp
#endif

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/expand_path.h>
#include <clientserver/stringUtils.h>
#include <clientserver/protocol.h>

#include "makeClientRequestBlock.h"
#include "startup.h"
#include "udaClient.h"
#include "getEnvironment.h"

int idamClientAPI(const char* file, const char* signal, int pass, int exp_number)
{
    REQUEST_BLOCK request_block;
    static short startup = 1;

    //-------------------------------------------------------------------------
    // Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    initRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    if (startup) {
        initIdamErrorStack();
        startup = 0;
    }

    // Create two regular arguments

    int err;
    char data_source[STRING_LENGTH + 1];
    if (strlen(file) == 0) {
        if (pass < 0)
            sprintf(data_source, "%d", exp_number);
        else
            sprintf(data_source, "%d/%d", exp_number, pass);
    } else {
        strcpy(data_source, file);
    }

    if ((err = makeClientRequestBlock(signal, data_source, &request_block)) != 0) {
        closeUdaError();
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
            addIdamError(CODEERRORTYPE, __func__, 999, "Error identifying the Data Source");
        }
        return -err;
    }

    //-------------------------------------------------------------------------
    // Fetch Data

    return idamClient(&request_block);
}

/*---------------------------------------------------------------
* Reads the Requested Data
*
* Input Arguments:	1) File Name
*			2) Signal Name
*			3) File Format
*			4) Plasma Pulse/Experiment Number
*
* Returns:
*
*--------------------------------------------------------------*/

int idamClientFileAPI(const char* file, const char* signal, const char* format)
{
    REQUEST_BLOCK request_block;
    static short startup = 1;

    //-------------------------------------------------------------------------
    // Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    initRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    if (startup) {
        initIdamErrorStack();
        startup = 0;
    }

    // Create two regular arguments

    int err;
    char data_source[STRING_LENGTH + 1];
    if (strlen(format) == 0)
        strcpy(data_source, file);
    else
        sprintf(data_source, "%s::%s", format, file);

    if ((err = makeClientRequestBlock(signal, data_source, &request_block)) != 0) {
        closeUdaError();
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
            addIdamError(CODEERRORTYPE, __func__, 999, "Error identifying the Data Source");
        }
        return -err;
    }

    //-------------------------------------------------------------------------

    UDA_LOG(UDA_LOG_DEBUG, "Routine: ClientFileAPI\n");
    UDA_LOG(UDA_LOG_DEBUG, "Routine: ClientFileAPI\n");

    UDA_LOG(UDA_LOG_DEBUG, "Number of Requests: %d\n", request_block.num_requests);
    for (int i = 0; i < request_block.num_requests; ++i) {
        auto req = &request_block.requests[i];
        UDA_LOG(UDA_LOG_DEBUG, "Request %d: %d\n", i, req->request);
        UDA_LOG(UDA_LOG_DEBUG, "File:       %d\n", i, req->path);
        UDA_LOG(UDA_LOG_DEBUG, "Signal:     %d\n", i, req->signal);
    }

    //-------------------------------------------------------------------------
    // Fetch Data

    return idamClient(&request_block);
}
