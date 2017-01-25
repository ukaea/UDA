/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from an IDAM server
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readIdam	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the IDAM server
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "readIdam.h"

#include <logging/idamLog.h>
#include <clientserver/idamErrorLog.h>

#ifdef NOIDAMPLUGIN

int readIdam(DATA_SOURCE data_source,
             SIGNAL_DESC signal_desc,
             REQUEST_BLOCK request_block,
             DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err, "Not Configured to Access the IDAM server Plugin.");
    return err;
}

#else

#include <stdlib.h>

#include <include/idamclientserverprivate.h>
#include <client/accAPI_C.h>
#include <clientserver/TrimString.h>
#include <clientserver/printStructs.h>
#include <client/IdamAPI.h>

#ifdef FATCLIENT

int readIdam(DATA_SOURCE data_source,
             SIGNAL_DESC signal_desc,
             REQUEST_BLOCK request_block,
             DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err, "Not Configured to Access the IDAM server Plugin.");
    return err;
}

#else

int readIdam(DATA_SOURCE data_source,
             SIGNAL_DESC signal_desc,
             REQUEST_BLOCK request_block,
             DATA_BLOCK* data_block)
{

    int handle, err = 0, oldport, newport;
    char* p;

    char signal[2 * MAXNAME + 2];
    char source[2 * MAXNAME + 2];
    char server_host[MAXNAME];

//----------------------------------------------------------------------
// Signal/Data Object & Data Source Details from the IDAM Database records

// Very primitive: Need to replicate fully the idamgetAPI arguments from the database

    if (request_block.request != REQUEST_READ_IDAM) {    // Not a Direct Request - Must be via the Database
        sprintf(signal, "%s::%s", data_source.archive, signal_desc.signal_name);
        sprintf(source, "%s::%d", data_source.device_name, data_source.exp_number);
    }

//----------------------------------------------------------------------
// Hand over Server IO File Handles to IDAM Client library

    resetIdamProperties();

    idamLog(LOG_DEBUG, "Plugin readIdam: Handing over Server File Handles to IDAM Client\n");
//----------------------------------------------------------------------
// Server Host and Port: Change if required

    strcpy(server_host, getIdamServerHost());        // Current Host
    oldport = getIdamServerPort();            // Current Port

    if (request_block.request != REQUEST_READ_IDAM) {
        if (data_source.server[0] != '\0') {
            if ((p = strstr(data_source.server, ":")) == NULL) {        // look for a port number in the server name
                p = strstr(data_source.server, " ");
            }
            if (p != NULL) {
                p[0] = '\0';
                if (strcasecmp(server_host, data_source.server) != 0) putIdamServerHost(data_source.server);
                if (IsNumber(&p[1])) {
                    newport = atoi(&p[1]);
                    if (newport != oldport) putIdamServerPort(atoi(&p[1]));
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err,
                                 "The Server Port must be an Integer Number passed "
                                         "using the formats 'server:port' or 'server port'");
                    return err;
                }
            } else {
                if (strcasecmp(server_host, data_source.server) != 0) putIdamServerHost(data_source.server);
            }
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err, "No Server has been specified!");
            return err;
        }
    } else {
        if (request_block.server[0] != '\0') {
            if ((p = strstr(request_block.server, ":")) == NULL) {        // look for a port number in the server name
                p = strstr(request_block.server, " ");
            }
            if (p != NULL) {
                p[0] = '\0';
                if (strcasecmp(server_host, request_block.server) != 0) putIdamServerHost(request_block.server);
                if (IsNumber(&p[1])) {
                    newport = atoi(&p[1]);
                    if (newport != oldport) putIdamServerPort(atoi(&p[1]));
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err,
                                 "The Server Port must be an Integer Number passed "
                                         "using the format 'server:port'  or 'server port'");
                    return err;
                }
            } else {
                if (strcasecmp(server_host, request_block.server) != 0) putIdamServerHost(request_block.server);
            }
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err, "No Server has been specified!");
            return err;
        }
    }

    idamLog(LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", getIdamServerHost());
    idamLog(LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", getIdamServerPort());

//----------------------------------------------------------------------
// Private Flags for the Server

    resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
    //dgm 02Oct14    setIdamPrivateFlag(PRIVATEFLAG_XDRFILE);		// Ensure Hierarchical Data are passed as an opaque file
    // BUG in the middleware - packageType corruption!

//dgm 03Aug2015		bug found elsewhere ... renable
    setIdamPrivateFlag(PRIVATEFLAG_XDRFILE);                // Ensure Hierarchical Data are passed as an opaque file

    if (environment.external_user) setIdamPrivateFlag(PRIVATEFLAG_EXTERNAL);    // Maintain external user status

//----------------------------------------------------------------------
// User Specified Flags and Properties for the Server

// Set Userid

// ... to be implemented

// Set Properties

    if (data_block->client_block.get_nodimdata) setIdamProperty("get_nodimdata");
    if (data_block->client_block.get_timedble) setIdamProperty("get_timedble");
    if (data_block->client_block.get_dimdble) setIdamProperty("get_dimdble");
    if (data_block->client_block.get_datadble) setIdamProperty("get_datadble");

    if (data_block->client_block.get_bad) setIdamProperty("get_bad");
    if (data_block->client_block.get_meta) setIdamProperty("get_meta");
    if (data_block->client_block.get_asis) setIdamProperty("get_asis");
    if (data_block->client_block.get_uncal) setIdamProperty("get_uncal");
    if (data_block->client_block.get_notoff) setIdamProperty("get_notoff");
    if (data_block->client_block.get_scalar) setIdamProperty("get_scalar");
    if (data_block->client_block.get_bytes) setIdamProperty("get_bytes");

// Timeout ...

// AltRank ...

// Client Flags ...

    resetIdamClientFlag(CLIENTFLAG_FULLRESET);
    setIdamClientFlag(data_block->client_block.clientFlags);

//----------------------------------------------------------------------
// Call for Data

    if (request_block.request != REQUEST_READ_IDAM) {
        idamLog(LOG_DEBUG, "readIdam: Calling idamGetAPI API (Database based Request)\n");
        idamLog(LOG_DEBUG, "readIdam Signal: %s\n", signal);
        idamLog(LOG_DEBUG, "readIdam Source: %s\n", source);

        handle = idamGetAPI(signal, source);

    } else {
        printRequestBlock(request_block);
        idamLog(LOG_DEBUG, "readIdam: Calling idamGetAPI API (Client based Request)\n");
        idamLog(LOG_DEBUG, "readIdam Signal: %s\n", request_block.signal);
        idamLog(LOG_DEBUG, "readIdam Source: %s\n", request_block.file);

// Re-attach Archive name to signal	**** better if the client didn't tokenise the input when calling an IDAM server

        if (protocolVersion < 6 && strlen(request_block.archive) != 0) {
            int lstr = (int) strlen(request_block.archive) + (int) strlen(request_block.signal) + 3;
            char* signal = (char*) malloc(lstr * sizeof(char));
            strcpy(signal, request_block.archive);
            strcat(signal, "::");
            strcat(signal, request_block.signal);
            handle = idamGetAPI((const char*) signal, (const char*) request_block.file);
            free((void*) signal);
        } else

            handle = idamGetAPI((const char*) request_block.signal, (const char*) request_block.file);

    }

    idamLog(LOG_DEBUG, "readIdam:Returned from idamGetAPI API: handle = %d, error code = %d\n", handle,
            getIdamErrorCode(handle));

//------------------------------------------------------------------------------
// Concatenate IDAM Client Error Message Stack

    concatIdamError(*getIdamServerErrorStack(), &idamerrorstack);

//----------------------------------------------------------------------
// Test for Errors: Close Socket and Free heap

    if (handle < 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", handle, "");
        return (abs(handle));
    } else {
        if ((err = getIdamErrorCode(handle)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readIdam", err, (char*) getIdamErrorMsg(handle));
            return err;
        }
    }

//----------------------------------------------------------------------
// Copy the Data Block

    *data_block = *getIdamDataBlock(handle);

    idamLog(LOG_DEBUG, "readIdam:\n");
    printDataBlock(*data_block);
    idamLog(LOG_DEBUG, "plugin readIdam: Exit\n");

//----------------------------------------------------------------------
// If the Data are Hierarchical, then necessary to forward the xdr file
//
// List of structures from external server may conflict with local definitions
//	Don't use local definitions
//
// No access to malloc log within client
//
// Data received is a Data Tree. This would need to be restructured - i.e., pointer extracted from
// structure SARRAY (may be different to local SARRAY!)
//	Don't pass a data tree - use an XDR file instead.
//	Required if http is to be adopted as middleware protocol
// 	Relay everything from the external server back to the client without interpretation.
//
// Namespace issues: Both the Client and the Server use the same functions to Query. The PRE_LOAD requirement of MDS+
// causes the IDAM client library to be loaded ahead of the server library: Result confusion and seg fault errors.
// Need to add unique name component to IDAM client server to provide namespace separation.
// Prepare a reduced set of external symbols for the client library attached to the server!

//----------------------------------------------------------------------
// House Keeping

    //idamFreeAll();		// Called when the server is about to die or after the data is moved to the client

    return 0;
}

#endif
#endif

