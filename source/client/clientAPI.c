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

#include <strings.h>

#include <logging/idamLog.h>
#include <include/idamclientprivate.h>
#include <include/idamclientserverprivate.h>
#include <clientserver/initStructs.h>
#include <clientserver/idamErrorLog.h>
#include <clientserver/expand_path.h>
#include <clientserver/stringUtils.h>

#include "makeClientRequestBlock.h"
#include "startup.h"
#include "idamClient.h"

int idamClientAPI(const char* file, const char* signal, int pass, int exp_number)
{
    REQUEST_BLOCK request_block;
    static short startup = 1;

//-------------------------------------------------------------------------
// Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

//-------------------------------------------------------------------------
// Log all Arguments passed from Application

#ifdef ARGSTACK
    if(argstack == NULL) {
        char tempFile[] = "/tmp/idamStackXXXXXX";
        mkstemp(tempFile);
        argstack = fopen(tempFile, environment.logmode);
        if(argstack != NULL) fprintf(argstack,"idamClientAPI\n");
    }
    if(argstack != NULL) {
        fprintf(argstack,"[%s][%s][%d][%d]\n", file, signal, exp_number, pass);
        fflush(argstack);
    }
#endif

//-------------------------------------------------------------------------
// Initialise the Client Data Request Structure

    initRequestBlock(&request_block);

//------------------------------------------------------------------------------
// Build the Request Data Block (Version and API dependent)

    if (startup) {
        initServerBlock(&server_block, 0);
        initIdamErrorStack(&idamerrorstack);
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
        concatIdamError(idamerrorstack, &server_block.idamerrorstack);
        closeIdamError(&idamerrorstack);
        if (server_block.idamerrorstack.nerrors == 0) {
            idamLog(LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
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
* Revision 0.0  05-Aug-2004	D.G.Muir
* 0.01  15Jun2006   D.G.Muir	netCDF Added
* 0.02  30Jan2007   D.G.Muir	Check for /scratch/ or /tmp/ directory files
* 0.03  09Mar2007   D.G.Muir	REQUEST_READ_HDF5 and REQUEST_READ_XML added
*--------------------------------------------------------------*/

int idamClientFileAPI(const char* file, const char* signal, const char* format)
{
    REQUEST_BLOCK request_block;
    static short startup = 1;

//-------------------------------------------------------------------------
// Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

//-------------------------------------------------------------------------
// Log all Arguments passed from Application

#ifdef ARGSTACK
    if(argstack == NULL) {
        char tempFile[] = "/tmp/idamStackXXXXXX";
        mkstemp(tempFile);
        argstack = fopen(tempFile, environment.logmode);
        if(argstack != NULL) fprintf(argstack,"idamClientFileAPI\n");
    }
    if(argstack != NULL) {
        fprintf(argstack,"[%s][%s][%d][%d]\n", file, signal, format);
        fflush(argstack);
    }
#endif

//-------------------------------------------------------------------------
// Initialise the Client Data Request Structure

    initRequestBlock(&request_block);

//------------------------------------------------------------------------------
// Build the Request Data Block (Version and API dependent)

    if (startup) {
        initServerBlock(&server_block, 0);
        initIdamErrorStack(&idamerrorstack);
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
        concatIdamError(idamerrorstack, &server_block.idamerrorstack);
        closeIdamError(&idamerrorstack);
        if (server_block.idamerrorstack.nerrors == 0) {
            idamLog(LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
        }
        return -err;
    }

//-------------------------------------------------------------------------

    idamLog(LOG_DEBUG, "Routine: ClientFileAPI\n");
    idamLog(LOG_DEBUG, "Routine: ClientFileAPI\n");
    idamLog(LOG_DEBUG, "Request 		 %d\n", request_block.request);
    idamLog(LOG_DEBUG, "File            %s\n", request_block.path);
    idamLog(LOG_DEBUG, "Signal  		 %s\n", request_block.signal);

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

    if (!strcasecmp(format, "ida")) {
        request_block.request = REQUEST_READ_IDA;
    } else if (!strcasecmp(format, "ppf")) {
        request_block.request = REQUEST_READ_PPF;
    } else if (!strcasecmp(format, "jpf")) {
        request_block.request = REQUEST_READ_JPF;
    } else {
        idamLog(LOG_ERROR, "The Specifed File Format [%s] is Not Supported\n", format);
        return FILE_FORMAT_NOT_SUPPORTED;
    }

//-------------------------------------------------------------------------
// Passed Args

    switch (request_block.request) {

        case REQUEST_READ_IDA:
            if (exp_number > 0)
                request_block.exp_number = exp_number;
            else
                request_block.exp_number = -1;    // Has No Meaning in this Context

            if (pass > -1)
                request_block.pass = pass;
            else
                request_block.pass = -1;        // Has No Meaning in this Context

            strcpy(request_block.file, "");
            strcpy(request_block.signal, signal);

            TrimString(request_block.path);
            expandFilePath(request_block.path);    // Expand Local Directory

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

    return (idamClient(&request_block));
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

    idamLog(LOG_DEBUG, "Routine: ClientTestAPI\n");
    idamLog(LOG_DEBUG, "Request           %d\n", request_block.request);
    idamLog(LOG_DEBUG, "File              %s\n", request_block.file);
    idamLog(LOG_DEBUG, "Signal            %s\n", request_block.signal);
    idamLog(LOG_DEBUG, "Pass              %d\n", request_block.pass);
    idamLog(LOG_DEBUG, "Experiment Number %d\n", request_block.exp_number);

//-------------------------------------------------------------------------
// Fetch Data

    return idamClient(&request_block);
}

