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
        closeIdamError();
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
        closeIdamError();
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
            addIdamError(CODEERRORTYPE, __func__, 999, "Error identifying the Data Source");
        }
        return -err;
    }

    //-------------------------------------------------------------------------

    UDA_LOG(UDA_LOG_DEBUG, "Routine: ClientFileAPI\n");
    UDA_LOG(UDA_LOG_DEBUG, "Routine: ClientFileAPI\n");
    UDA_LOG(UDA_LOG_DEBUG, "Request 		 %d\n", request_block.request);
    UDA_LOG(UDA_LOG_DEBUG, "File            %s\n", request_block.path);
    UDA_LOG(UDA_LOG_DEBUG, "Signal  		 %s\n", request_block.signal);

    //-------------------------------------------------------------------------
    // Fetch Data

    return idamClient(&request_block);
}

/*---------------------------------------------------------------
* Reads the Requested Data
*
* Input Arguments:	1) File Name
*			2) Format
*			3) Ownerid
*			4) Signal Name
*			5) Plasma Pulse/Experiment Number
*			6) Pass/Sequence
*
* Returns:
*
* Revision 0.0  18 Jan 2006	D.G.Muir
*
*--------------------------------------------------------------*/

int idamClientFileAPI2(const char* file, const char* format, const char* owner,
                       const char* signal, int exp_number, int pass)
{
    REQUEST_BLOCK request_block;

    //-------------------------------------------------------------------------
    // Open the Logs

    if (idamStartup(0) != 0) return (PROBLEM_OPENING_LOGS);

    //-------------------------------------------------------------------------
    // Test Specified Format: Convert to Lower Case

    initRequestBlock(&request_block);

    if (STR_IEQUALS(format, "ida")) {
        request_block.request = REQUEST_READ_IDA;
    } else if (STR_IEQUALS(format, "ppf")) {
        request_block.request = REQUEST_READ_PPF;
    } else if (STR_IEQUALS(format, "jpf")) {
        request_block.request = REQUEST_READ_JPF;
    } else {
        UDA_LOG(UDA_LOG_ERROR, "The Specifed File Format [%s] is Not Supported\n", format);
        return FILE_FORMAT_NOT_SUPPORTED;
    }

    //-------------------------------------------------------------------------
    // Passed Args

    switch (request_block.request) {

        case REQUEST_READ_IDA:
            if (exp_number > 0) {
                request_block.exp_number = exp_number;
            } else {
                // Has No Meaning in this Context
                request_block.exp_number = -1;
            }

            if (pass > -1) {
                request_block.pass = pass;
            } else {
                // Has No Meaning in this Context
                request_block.pass = -1;
            }

            strcpy(request_block.file, "");
            strcpy(request_block.signal, signal);

            TrimString(request_block.path);
            expandFilePath(request_block.path, getIdamClientEnvironment());    // Expand Local Directory

            break;

        case REQUEST_READ_PPF:
            request_block.exp_number = exp_number;
            request_block.pass = pass;
            strcpy(request_block.file, file);
            strcpy(request_block.signal, signal);
            strcpy(request_block.path, owner);
            break;

        case REQUEST_READ_JPF:
            request_block.exp_number = exp_number;
            strcpy(request_block.file, file);
            strcpy(request_block.signal, signal);
            break;
    }

    //-------------------------------------------------------------------------
    // Fetch Data

    return idamClient(&request_block);
}


int idamClientTestAPI(const char* file, const char* signal, int pass, int exp_number)
{
    REQUEST_BLOCK request_block;

    //-------------------------------------------------------------------------
    // Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

    //-------------------------------------------------------------------------
    // Passed Args

    initRequestBlock(&request_block);

    //request_block.request    = REQUEST_READ_NEW_PLUGIN ;
    request_block.request = REQUEST_READ_NOTHING;
    request_block.exp_number = exp_number;
    request_block.pass = pass;

    strcpy(request_block.file, file);
    strcpy(request_block.signal, signal);

    UDA_LOG(UDA_LOG_DEBUG, "Routine: ClientTestAPI\n");
    UDA_LOG(UDA_LOG_DEBUG, "Request           %d\n", request_block.request);
    UDA_LOG(UDA_LOG_DEBUG, "File              %s\n", request_block.file);
    UDA_LOG(UDA_LOG_DEBUG, "Signal            %s\n", request_block.signal);
    UDA_LOG(UDA_LOG_DEBUG, "Pass              %d\n", request_block.pass);
    UDA_LOG(UDA_LOG_DEBUG, "Experiment Number %d\n", request_block.exp_number);

    //-------------------------------------------------------------------------
    // Fetch Data

    return idamClient(&request_block);
}

