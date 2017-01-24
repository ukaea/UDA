/*---------------------------------------------------------------
* IDAM Data Server
*
* Note: Error Message Passback to Client is via the Server State Structure
*
*----------------------------------------------------------------
* Change History:
*
* 0.3  10Feb2005   D.G.Muir
* 0.4  10Jul2005   D.G.Muir
*
* 1.01 15Jun2006  dgmuir	readCDF file added
* 1.02 29Jun2006  dgmuir	Generic readCDF added
* 1.03 02Aug2006  dgmuir	Tidy-up
* 1.04 05Sep2006  dgmuir	Status datatype changed from Byte to Int
*				Generic Data Reader + XML Composite Signal + XML Actions
* 1.05 26Oct2006  dgmuir	Performance Optimised by preventing a system call for the userid
*				on initialising the Client structure: A global is used instead - saves 10ms!!!.
*				The changeover to a terse client server conversation is slower than the original verbose!!
* 1.06 22Jan2007  dgmuir	Server-Side Data processing added: get_timedble and get_scalar
* 1.07 09Mar2007  dgmuir	HDF5 File and XML Signal Composite plugins added
* 1.08 14Mar2007  dgmuir	UFile plugin added
* 1.09 15Mar2007  dgmuir	General File Format request added - Server chooses appropriate plugin
* 1.10 21Mar2007  dgmuir	user_timeout renamed server_timeout
*				accout renamed server_accout
*				tot_block_time renamed server_tot_block_time
* 1.11 21Mar2007  dgm		XDR streams renamed prefixed with server
* 1.12 22Mar2007  dgm		performance renamed serverPerformance
* 1.13 27Mar2007  dgm		Data Source File Handle Management added
* 1.14 10Apr2007  dgm		FATCLIENT and GENERIC_ENABLE Compiler Options Added
* 1.15 16Apr2007  dgm		Signal_Rec explicitly initialised prior to idamserverGetData
* 1.16 17Apr2007  dgm		IDA_ENABLE Compiler Options Added
* 1.17 14May2007  dgm		Extended REQUEST_READ_NOTHING to use idamGetAPI API
* 1.18 31May2007  dgm		Hierarchical Data Added
* 1.19 09Jul2007  dgm		globals: debugon and verbose added if NOT a FAT Client
* 1.20 18Oct2007  dgm		idamAccessLog implemented
* 1.21 22Oct2007  dgm		idamErrorLog implemented
* 1.22 12Mar2008  dgm		initDataBlock moved to start of routine rather than immediately prior to idamserverGetData
*				because it is called by idamAccessLog before this!
* 1.23 12Mar2008  dgm		initActions moved to start of routine: An early error attempts to free uninitialised structures
* 1.24 28Mar2008  dgm		Prevented copying of signal names that are too long using the copyString function
* 04Nov2008	  dgm		protocolVersion added: Assigned using the client version number
*				Added named Protocol IDs
*				Modified the Hierarchical Data Structure component
* 10Dec2008	dgm	Server version set to 3: this avoids problems with protocol configuration for old clients.
* 08Jul2009	dgm	This function is now included in the shared library: name changed from main to idamServer.
* 23Sep2009	dgm	Moved the check on data types and client version out of protocol.c. Error message added
*			to Server_Block before any attempt to transfer data: prevents a HANG.
* 30SEP2009	dgm	Test the path for a TARGET sub-component and make a substitution if found.
*			Uses environment.private_path_target and environment.private_path_substitute
*			e.g target /.automount/funsrv1/root/home and substitute /net/funsrv1/home
* 07Oct2009	dgm	Added external_user to ENVIRONMENT structure (same effect as compiler option EXTERNAL_USER)
* 10Oct2009	dgm	external_user functionality moved to readBytes plugin.
* 23Nov2009	dgm	Added SQL Archive plugin (Not Continuous Measured Data)
* 25Nov2009	dgm	Added generalised user defined data structure passing
*			Added additional protocol version test for compound types - all tests consolidated into protocolVersionTypeTest
*			Changed the server version number from 3 to 4
* 01Mar2010	dgm	Changes for fatclient to access generalised data structures. XDR streams are replaced with XDR
*			files written and read from /tmp files. Not efficient - best approach is to gererate data trees
*			directly without needing XDR.
* 04mar2010	dgm	Added static SQL socket gDBConnect to pass back from idamServerGetData (passed by value).
* 23Apr2010	dgm	Increased server version # to 5 from 4
*			Hierarchical Data passed as an Opaque binary object when requested by the client
*			Necessary for http and IDAM->IDAM data access
* 26Apr2010	dgm	XDRstdioFlag added to indicate whether or not the XDR stream is to/from a file
* 11May2010	dgm	source and api_delim added to REQUEST_BLOCK: decoding client arguments now possible within
*			server, decoupling server further from client (apart from legacy APIs).
*			Server version incremented to 6 dependent on TESTCODE
* 26May2010	dgm	PLUGINLIST pluginList global structure added
* 02Nov2010	dgm	clientFlags and altRank added - passed via CLIENT_BLOCK
* 12Apr2011	dgm	Added test for PRIVATEFLAG_EXTERNAL passed from originating server: enable external user access policy
* 24May2011	dgm	Choose the lower of the client and server version numbers for the protocol version
* 02Jun2011	dgm	IDA_ENABLE compiler option removed
* 05Sep2011	dgm	Free heap containing name value pairs using freeNameValueList
* 19Sep2011	dgm	Fixed bug: idamerrorstack defined twice when SERVERBUILD macro defined
*			Add accessors to local globals: idamerrorstack, userdefinedtypelist,
*			logmalloclist, parseduserdefinedtypelist
// 12Mar2012	dgm	Removed TESTCODE compiler option - legacy code deleted.
// 10Oct2012	dgm	Corrected bug: removed fullNTree initialisation
// 06Nov2012	dgm	Added global variables lastMallocIndex and lastMallocIndexValue
// 20Mar2013	dgm	Changed name: startup.c to idamServerStartup.c
// 02Oct2013	dgm	idamServerPluginHelp no longer required - functionality delivered via an external plugin
// 18Nov2013	dgm	PUTDATA functionality included as standard rather than with a compiler option
// 20Nov2013	dgm	PLUGINTEST, DEV_08NOV2012 selections commented out
// 28Nov2013	dgm	To preserve backwards compatibility with client and server running protocol versions <= 6
//			divert all client server communications and data access to a legacy server
// 24Apr2014	dgm	Added Global unsigned int totalDataBlockSize to hold the amount of data sent for the last request
//			Added explicit calculation via function countDataBlockSize - Legacy structures only
//			Added Global unsigned int cachePermission to return True/False - OK for client to cache?
//			Both totalDataBlockSize and cachePermission are returned to the client via the SERVER_BLOCK
// 10Jul2014	dgm	Added Test versions of idamServerPlugin and makeServerRequestBlock when compiler macro DGMJUL14 is defined.
// 18Mar2015	dgm	Added getIdamServerParsedUserDefinedTypeList()
*---------------------------------------------------------------------------------------------------------------------*/
#include <parseIncludeFile.h>
#include <clientserver/idamErrors.h>
#include "idamserver.h"

#include "idamserverfiles.h"
#include "idamServerStartup.h"
#include "idamErrorLog.h"
#include "protocol2.h"
#include "CreateXDRStream.h"
#include "initStructs.h"
#include "manageSockets.h"
#include "printStructs.h"
#include "closeServerSockets.h"
#include "serverProcessing.h"
#include "idamAccessLog.h"
#include "idamserverGetData.h"
#include "freeIdamPut.h"
#include "idamLegacyServer.h"
#include "idamServerPlugin.h"
#include "idamServerLegacyPlugin.h"
#include "struct.h"
#include "makeServerRequestBlock.h"
#include "freeDataBlock.h"
#include "manageFiles.h"

#ifndef FATCLIENT
#  include "xdrlib.h"
#  ifdef SECURITYENABLED
#    include "idamsecurity.h"
#  endif
#  ifndef NOTGENERICENABLED
#  endif
#  ifdef NONETCDFPLUGIN
void ncclose(int fh) {
    return;
}
#  endif
#  ifdef GENERALSTRUCTS
#  endif
#else
#  include "copyStructs.h"
#  include "protocolXML.h"
#endif

PGconn* gDBConnect = NULL;    // IDAM SQL database Socket Connection pass back fix

#ifdef FATCLIENT
extern PGconn * DBConnect;        // IDAM SQL database Socket Connection
PGconn * DBConnect = NULL;
#endif

//--------------------------------------------------------------------------------------
// static globals

static XDR serverXDRInput;
static XDR serverXDROutput;

XDR* serverInput = &serverXDRInput;
XDR* serverOutput = &serverXDROutput;

int server_tot_block_time = 0;

int serverVersion = 7;
int protocolVersion = 7;
#ifndef FATCLIENT
static int legacyServerVersion = 6;
#endif

IDAMFILELIST idamfilelist;
NTREELIST NTreeList;
NTREE* fullNTree = NULL;
LOGSTRUCTLIST logstructlist;

#ifdef SECURITYENABLED
//static int serverVersion = 18;
#endif

char serverUsername[STRING_LENGTH] = "server";

#ifdef SERVERBUILD
FILE *dbgout = NULL;		// Debug Log
FILE *errout = NULL;		// Error Log

IDAMERRORSTACK idamerrorstack;
#endif

int server_timeout = TIMEOUT;        // user specified Server Lifetime

#ifdef PERFORMANCETEST
static PERFORMANCE serverPerformance;		// Performance Diagnostics
static PERFORMANCE serverPerformance2;
#endif

#ifndef FATCLIENT
#ifdef GENERALSTRUCTS
USERDEFINEDTYPELIST* userdefinedtypelist = NULL;        // User Defined Structure Types from Data Files & Plugins
LOGMALLOCLIST* logmalloclist = NULL;            // List of all Heap Allocations for Data: Freed after data is dispatched
int malloc_source = MALLOCSOURCENONE;
USERDEFINEDTYPELIST parseduserdefinedtypelist;        // Initial set of User Defined Structure Types
unsigned int lastMallocIndex = 0;            // Malloc Log search index last value
unsigned int* lastMallocIndexValue = &lastMallocIndex;    // Preserve Malloc Log search index last value in GENERAL_STRUCT
#endif

unsigned int totalDataBlockSize = 0;            // Total amount sent for the last data request
//static unsigned int cachePermission = PLUGINNOTOKTOCACHE;	// No Permission for Client to Cache returned data

unsigned int clientFlags = 0;
int altRank = 0;

unsigned int privateFlags = 0;
unsigned int XDRstdioFlag = 0;                // Flags the XDR stream is to stdio not a socket

#endif

#ifdef SERVERBUILD
PLUGINLIST pluginList;					// List of all data reader plugins (internal and external shared libraries)
#endif

#ifndef FATCLIENT
PLUGINLIST pluginList;                // List of all data reader plugins (internal and external shared libraries)
ENVIRONMENT environment;                // Holds local environment variable values
#endif

FILE* server_accout;
SOCKETLIST server_socketlist;

//--------------------------------------------------------------------------------------
// Accessors to local globals for use by plugins

IDAMERRORSTACK* getIdamServerPluginErrorStack()
{
    return &idamerrorstack;
}

USERDEFINEDTYPELIST* getIdamServerUserDefinedTypeList()
{
    return userdefinedtypelist;
}

USERDEFINEDTYPELIST* getIdamServerParsedUserDefinedTypeList()
{
    return &parseduserdefinedtypelist;
}

LOGMALLOCLIST* getIdamServerLogMallocList()
{
    return logmalloclist;
}

void putIdamServerErrorStack(IDAMERRORSTACK errorstack)
{
    idamerrorstack = errorstack;
}

void putIdamServerUserDefinedTypeList(USERDEFINEDTYPELIST* definedtypelist)
{
    userdefinedtypelist = definedtypelist;
}

void putIdamServerLogMallocList(LOGMALLOCLIST* malloclist)
{
    logmalloclist = malloclist;
}

void putIdamServerEnvironment(ENVIRONMENT environ)
{
    environment = environ;    // Replace the server's environment
}

extern void copyIdamServerEnvironment(ENVIRONMENT* environ)
{
    *environ = environment;    // Copy the server's environment structure
}

//--------------------------------------------------------------------------------------
// Server Entry point

#ifndef FATCLIENT

extern int idamServer(int argc, char** argv)
{
#else
#include <assert.h>
    int idamServer(CLIENT_BLOCK client_block, REQUEST_BLOCK * request_block0, SERVER_BLOCK * server_block0,
                          DATA_BLOCK * data_block0)
    {
        assert(data_block0 != NULL);
#endif

    int i, rc, err = 0, depth, fatal = 0;
    int protocol_id;

    static int socket_list_initialised = 0;
    static int file_list_initialised = 0;
    static int plugin_list_initialised = 0;

    char* token = NULL;

    static int fileParsed = 0;

#ifndef FATCLIENT
    PGconn* DBConnect = NULL;
    int next_protocol;

    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
#else
#  ifndef NOTGENERICENABLED
    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
#  endif
#endif

    DATA_SOURCE data_source;
    SIGNAL signal_rec;
    SIGNAL_DESC signal_desc;

    DATA_BLOCK data_block;
    REQUEST_BLOCK request_block;
#ifndef FATCLIENT
    CLIENT_BLOCK client_block;
#endif
    SERVER_BLOCK server_block;

    ACTIONS actions_desc;
    ACTIONS actions_sig;

#ifdef PERFORMANCETEST
    serverPerformance.npoints = 1;  	// Max is 10 so set at end!
    strcpy(serverPerformance.label[0],"#0: Beginning of Server Startup");
    rc = gettimeofday(&(serverPerformance.tv_start[0]), NULL);
#endif

//-------------------------------------------------------------------------
// Initialise the Error Stack & the Server Status Structure
// Reinitialised after each logging action

#ifndef FATCLIENT
    initIdamErrorStack(&idamerrorstack);
#endif

    initServerBlock(&server_block, serverVersion);
    initDataBlock(&data_block);
    initActions(&actions_desc);        // There may be a Sequence of Actions to Apply
    initActions(&actions_sig);

//-------------------------------------------------------------------------
// Create the Server Log Directory: Fatal Error if any Problem Opening a Log?

#ifndef FATCLIENT    // <========================== Client Server Code Only

    if ((rc = startup()) != 0) {
        fatal = 1;
        err = FATAL_ERROR_LOGS;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Fatal Error Opening the Server Logs");
        server_block.idamerrorstack = idamerrorstack;
        initIdamErrorStack(&idamerrorstack);
    }

    IDAM_LOG(LOG_DEBUG, "New Server Instance\n");

//-------------------------------------------------------------------------
// Create the XDR Record Streams

    CreateXDRStream();

    IDAM_LOG(LOG_DEBUG, "XDR Streams Created\n");

#endif			// <========================== End of Client Server Code 

//-------------------------------------------------------------------------
// Open and Initialise the Socket List (Once Only)

    if (!socket_list_initialised) {
        initSocketList(&server_socketlist);
        socket_list_initialised = 1;
    }

//-------------------------------------------------------------------------
// Open and Initialise the File Handle List (Once Only)

#ifdef FILELISTTEST
    if(!file_list_initialised) {
        initIdamFileList(&idamfilelist);
        file_list_initialised = 1;
    }
#endif

//----------------------------------------------------------------------
// Initialise General Structure Passing

#ifdef GENERALSTRUCTS

// this step needs doing once only - the first time a generalised user defined structure is encountered.
// For FAT clients use a static state variable to prevent multiple parsing

    if (!fileParsed) {
        fileParsed = 1;
        initUserDefinedTypeList(&parseduserdefinedtypelist);
        userdefinedtypelist = &parseduserdefinedtypelist;                    // Switch before Parsing input file

        if ((token = getenv("IDAM_SARRAY_CONFIG")) == NULL) {
            // ERROR !!!
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "No Environment variable IDAM_SARRAY_CONFIG");
        } else {
            rc = parseIncludeFile(token);                // file containing the SARRAY structure definition
            parseduserdefinedtypelist = *userdefinedtypelist;                    // Switch back
        }
    }

    userdefinedtypelist = NULL;                                // Startup State

#endif

//----------------------------------------------------------------------
// Initialise the Data Reader Plugin list

    if (!plugin_list_initialised) {
        pluginList.count = 0;
        initPluginList(&pluginList);
        plugin_list_initialised = 1;

        IDAM_LOG(LOG_INFO, "List of Plugins available\n");
        for (i = 0; i < pluginList.count; i++) {
            IDAM_LOGF(LOG_INFO, "[%d] %d %s\n", i, pluginList.plugin[i].request, pluginList.plugin[i].format);
        }
    }

#ifdef PERFORMANCETEST
    rc = gettimeofday(&(serverPerformance.tv_end[0]), NULL);
#endif

//----------------------------------------------------------------------------
// Server Information: Operating System Name - may limit types of data that can be received by the Client

    char* env = NULL;

    if ((env = getenv("OSTYPE")) != NULL)
        strcpy(server_block.OSName, env);
    else if ((env = getenv("IDAM_SERVER_OS")) != NULL) strcpy(server_block.OSName, env);

// Server Configuration and Environment DOI

    if ((env = getenv("IDAM_SERVER_DOI")) != NULL) strcpy(server_block.DOI, env);

//-------------------------------------------------------------------------
// User Authentication at startup

#ifndef FATCLIENT    // <========================== Client Server Code Only

#ifdef SECURITYENABLED
    /*
          static int authenticationNeeded = 1;			// No data access until this is set TRUE

          initClientBlock(&client_block, 0, "");

          if(authenticationNeeded && protocolVersion >= IDAMSECURITYVERSION){

    // User or intermediate server Must Authenticate
    // If the request is passed on through a chain of servers, user#2 authentication occurs within the external server.
    // An authentication structure is passed back from the server to the client


    // Receive the client_block
    // Test data validity of certificate
    // Test certificate signature						=> has a valid certificate (not proof of authentication)
    // Decrypt token A with the server private key
    // Encrypt token A with the client public key
    // Generate new token B and encrypt with the client public key

             unsigned short authenticationStep = 2;

         if((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0){
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Authentication Failed #2");
            return err;
         }

             authenticationStep = 3;

         if((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0){
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Client or Server Authentication Failed #1");
            return err;
         }

    // Send the server_block

             authenticationStep = 4;

         if((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0){
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Client or Server Authentication Failed #1");
            return err;
         }

    // Receive the client_block
    // Decrypt token B with the server private key				=> Proof Client has valid private key == client authenticated
    // Test token B identical to that sent in step 4
    // Generate a new token B and encrypt with the client public key	=> maintain mutual authentication
    // Send the server_block

             authenticationStep = 7;

         if((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0){
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Client or Server Authentication Failed #1");
            return err;
         }

         authenticationNeeded = 0;
          }

    #undef IDAMSECURITY 	// Old macro superceded
    */
#else

// Exchange version details - once only

    initClientBlock(&client_block, 0, "");

//----------------------------------------------------------------------------
// Initialise the Request Structure

    initRequestBlock(&request_block);

// Receive the client block, respecting earlier protocol versions

    IDAM_LOG(LOG_DEBUG, "Waiting for Initial Client Block\n");

    if (!(rc = xdrrec_skiprecord(serverInput))) {
        err = PROTOCOL_ERROR_5;
        IDAM_LOG(LOG_DEBUG, "xdrrec_skiprecord error!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block)");
    } else {

        protocol_id = PROTOCOL_CLIENT_BLOCK;        // Recieve Client Block

        if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, &client_block)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 10 Error (Client Block)");
            IDAM_LOG(LOG_DEBUG, "protocol error! Client Block not received!\n");
        }

        if (err == 0) {
            IDAM_LOG(LOG_DEBUG, "Initial Client Block received\n");
            printClientBlock(client_block);
        }

// Test for an immediate CLOSEDOWN instruction

        if (client_block.timeout == 0 || client_block.clientFlags & CLIENTFLAG_CLOSEDOWN) goto SERVERCLOSEDOWN;

    }

    if (err != 0) return err;

// Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

// Protocol Version: Lower of the client and server version numbers
// This defines the set of elements within data structures passed between client and server
// Must be the same on both sides of the socket
// set in xdr_client

    //protocolVersion = serverVersion;
    //if(client_block.version < serverVersion) protocolVersion = client_block.version;
    //if(client_block.version < server_block.version) protocolVersion = client_block.version;

// Send the server block

    IDAM_LOG(LOG_DEBUG, "Sending Initial Server Block \n");
    printServerBlock(server_block);

    protocol_id = PROTOCOL_SERVER_BLOCK;        // Receive Server Block: Server Aknowledgement

    if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &server_block)) != 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 11 Error (Server Block #1)");
        return err;
    }

    if (!(rc = xdrrec_endofrecord(serverOutput, 1))) {    // Send data now
        err = PROTOCOL_ERROR_7;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Server Block)");
        return err;
    }

    IDAM_LOG(LOG_DEBUG, "Initial Server Block sent without error\n");

// If the protocol version is legacy (<=6), then divert full control to a legacy server

    if (client_block.version <= legacyServerVersion) {
        IDAM_LOG(LOG_DEBUG, "Diverting to the Legacy Server\n");
        IDAM_LOGF(LOG_DEBUG, "Client protocol %d\n", client_block.version);
        return idamLegacyServer(client_block);
    }


#endif // not SECURITYENABLED

//----------------------------------------------------------------------------
// Start of Server Wait Loop

    do {
        IDAM_LOG(LOG_DEBUG, "IdamServer: Start of Server Wait Loop\n");

#endif			// <========================== End of Client Server Code 


#ifdef PERFORMANCETEST
        serverPerformance.npoints = 3;
        strcpy(serverPerformance.label[1],"#1: Beginning of Main Server Loop");
        rc = gettimeofday(&(serverPerformance.tv_start[1]), NULL);
        strcpy(serverPerformance.label[2],"#2: Client Request");
        rc = gettimeofday(&(serverPerformance.tv_start[2]), NULL);
#endif

//----------------------------------------------------------------------------
// Start of Error Trap Loop #1

        do {
            IDAM_LOG(LOG_DEBUG, "IdamServer: Start of Server Error Trap #1 Loop\n");

#ifdef FATCLIENT    // <========================== Fat Client Code Only
            copyRequestBlock(&request_block, *request_block0);
#endif

//----------------------------------------------------------------------------
// Client and Server States
//
// Errors: Fatal to Data Access
//	   Pass Back and Await Client Instruction

#ifndef FATCLIENT    // <========================== Client Server Code Only
            initClientBlock(&client_block, 0, "");

            IDAM_LOG(LOG_DEBUG, "IdamServer: Waiting to receive Client Block\n");

// Receive the Client Block, request block and putData block

            if (!(rc = xdrrec_skiprecord(serverInput))) {
                err = PROTOCOL_ERROR_5;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block)");
                fatal = 1;
                break;
            }

            protocol_id = PROTOCOL_CLIENT_BLOCK;

            if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, &client_block)) != 0) {

                if (server_tot_block_time >= 1000 * server_timeout) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Server Time Out");
                    IDAM_LOGF(LOG_DEBUG, "Server Time Out  [%d]\n", server_tot_block_time);
                    fatal = 1;
                    break;
                }

                IDAM_LOG(LOG_DEBUG, "Problem Receiving Client Data Block\n");

                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "Protocol 10 Error (Receiving Client Block)");
                concatIdamError(idamerrorstack,
                                &server_block.idamerrorstack);        // Update Server State with Error Stack
                closeIdamError(&idamerrorstack);

                fatal = 1;
                break;
            }

            server_timeout = client_block.timeout;        // User specified Server Lifetime
            privateFlags = client_block.privateFlags;    // Server to Server flags
            clientFlags = client_block.clientFlags;    // Client set flags
            altRank = client_block.altRank;            // Rank of Alternative source

// Protocol Version: Lower of the client and server version numbers
// This defines the set of elements within data structures passed between client and server
// Must be the same on both sides of the socket

            protocolVersion = serverVersion;
            if (client_block.version < serverVersion) protocolVersion = client_block.version;

// The client request may originate from a server.
// Is the Originating server an externally facing server? If so then switch to this mode: preserve local access policy

//#ifndef FATCLIENT
            if (!environment.external_user && (privateFlags & PRIVATEFLAG_EXTERNAL)) environment.external_user = 1;
//#endif

            IDAM_LOGF(LOG_DEBUG, "client protocolVersion %d\n", protocolVersion);
            IDAM_LOGF(LOG_DEBUG, "privateFlags %d\n", privateFlags);
            IDAM_LOGF(LOG_DEBUG, "clientFlags  %d\n", clientFlags);
            IDAM_LOGF(LOG_DEBUG, "altRank      %d\n", altRank);
            IDAM_LOGF(LOG_DEBUG, "external?    %d\n", environment.external_user);

#ifndef ERRORSTACK
            if(server_block.idamerrorstack.nerrors > 0) {
                server_block.error = server_block.idamerrorstack.idamerror[0].code;
                strcpy(server_block.msg,server_block.idamerrorstack.idamerror[0].msg);
            }
#endif

// Test the client version is compatible with this server version

            if (protocolVersion > serverVersion) {
                IDAM_LOG(LOG_DEBUG,
                        "Client API Version is Newer than the Server Version: Update this Server!!!\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "Protocol Error: Client API Version is Newer than the Server Version");
                break;
            }

            if (fatal) {
                if (server_block.idamerrorstack.nerrors > 0)
                    err = server_block.idamerrorstack.idamerror[0].code;
                else
                    err = 1;
                break;                // Manage the Fatal Server State
            }

// Test for an immediate CLOSEDOWN instruction

            if (client_block.timeout == 0 || client_block.clientFlags & CLIENTFLAG_CLOSEDOWN) goto SERVERCLOSEDOWN;
#endif			// <========================== End of Client Server Code 

//-------------------------------------------------------------------------
// Client Request
//
// Errors: Fatal to Data Access
//	   Pass Back and Await Client Instruction

#ifndef FATCLIENT    // <========================== Client Server Code Only

            protocol_id = PROTOCOL_REQUEST_BLOCK;

            if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, &request_block)) != 0) {
                IDAM_LOG(LOG_DEBUG, "Problem Receiving Client Request Block\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "Protocol 1 Error (Receiving Client Request)");
                break;
            }

            IDAM_LOG(LOG_DEBUG, "Request Block Received\n");

#endif			// <========================== End of Client Server Code

            printClientBlock(client_block);
            printServerBlock(server_block);
            printRequestBlock(request_block);


//------------------------------------------------------------------------------------------------------------------
// Prepend Proxy Host to Source to redirect client request

            /*! On parallel clusters where nodes are connected together on a private network, only the master node may have access
            to external data sources. Cluster nodes can access these external sources via an IDAM server running on the master node.
            This server acts as a proxy server. It simply redirects requests to other external IDAM servers. To facilitate this redirection,
            each access request source string must be prepended with "IDAM::host:port/" within the proxy server. The host:port component is defined
            by the system administrator via an environment variable "IDAM_PROXY". Client's don't need to specifiy redirection via a Proxy - it's
            automatic if the IDAM_PROXY environment variable is defined. No prepending is done if the source is already a redirection, i.e. it
            begins "IDAM::".

            The name of the proxy reirection plugin is IDAM by default but may be changed using the environment variable IDAM_PROXYPLUGINNAME
            */

#ifdef PROXYSERVER

#ifdef DGM8OCT14

            // Name of the Proxy plugin

                        char *proxyNameDefault = "IDAM";
                        char *proxyName        = NULL;

                        char work[STRING_LENGTH];

                        if((proxyName = getenv("IDAM_PROXYPLUGINNAME")) == NULL) proxyName = proxyNameDefault;

            // Check string length compatibility

                        if(strlen(request_block.source) >= (STRING_LENGTH-1 - strlen(proxyName) - strlen(environment.server_proxy) - strlen(request_block.api_delim))) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "PROXY redirection: The source argument string is too long!");
                            break;
                        }

            // Prepend the client request and test for a redirection request via the proxy's plugin

                        if(request_block.api_delim[0] != '\0')
                            sprintf(work, "%s%s", proxyName, request_block.api_delim);
                        else
                            sprintf(work, "%s%s", proxyName, environment.api_delim);

                        if(strncasecmp(request_block.source, work, strlen(work)) != 0) {		// Not a recognised redirection so prepending is necessary

            // Has a proxy host been specified in the server startup script? If not assume the plugin has a default host and port

                            if(environment.server_proxy[0] == '\0') {
                                if(request_block.api_delim[0] != '\0')
                                    sprintf(work, "%s%s%s", proxyName, request_block.api_delim, request_block.source);		// IDAM::source
                                else
                                    sprintf(work, "%s%s%s", proxyName, environment.api_delim, request_block.source);

                                strcpy(request_block.source, work);

                            } else {												// IDAM::host.port/source

            // Check the Server Version is Compatible with the Originating client version ?

                                if(client_block.version < 6) {
                                    err = 999;
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                                 "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
                                    break;
                                }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The IDAM Plugin strips out the host and port data from the source so the originating server details are never passed.
            // Primitive test as the same IP address can be mapped to different names!
            // Should pass on the number of redirections and cap the limit!

                                if(environment.server_this[0] != '\0') {
                                    if(request_block.api_delim[0] != '\0')
                                        sprintf(work, "%s%s%s", proxyName, request_block.api_delim, environment.server_this);
                                    else
                                        sprintf(work, "%s%s%s", proxyName, environment.api_delim, environment.server_this);

                                    if((token=strstr(request_block.source, work)) != NULL) {
                                        err = 999;
                                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                                     "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                                        break;
                                    }
                                }

            // Prepend the redirection IDAM server details and replace the original

                                if(request_block.source[0] == '/') {
                                    if(request_block.api_delim[0] != '\0')
                                        sprintf(work, "%s%s%s%s", proxyName, request_block.api_delim, environment.server_proxy, request_block.source);
                                    else
                                        sprintf(work, "%s%s%s%s", proxyName, environment.api_delim, environment.server_proxy, request_block.source);
                                } else {
                                    if(request_block.api_delim[0] != '\0')
                                        sprintf(work, "%s%s%s/%s", proxyName, request_block.api_delim, environment.server_proxy, request_block.source);
                                    else
                                        sprintf(work, "%s%s%s/%s", proxyName, environment.api_delim, environment.server_proxy, request_block.source);
                                }
                                strcpy(request_block.source, work);
                            }

                            if(debugon) {
                                IDAM_LOG(LOG_DEBUG, "PROXY Redirection to %s avoiding %s\n", environment.server_proxy, environment.server_this);
                                IDAM_LOG(LOG_DEBUG, "plugin: %s\n", proxyName);
                                IDAM_LOG(LOG_DEBUG, "source: %s\n", request_block.source);
                            }
                        }

#else

                        if(request_block.api_delim[0] != '\0')
                            sprintf(work, "IDAM%s", request_block.api_delim);
                        else
                            sprintf(work, "IDAM%s", environment.api_delim);

                        if(environment.server_proxy[0] != '\0' && strncasecmp(request_block.source, work, strlen(work)) != 0) {

            // Check the Server Version is Compatible with the Originating client version ?

                            if(client_block.version < 6) {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                             "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
                                break;
                            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The IDAM Plugin strips out the host and port data from the source so the originating server details are never passed.

                            if(request_block.api_delim[0] != '\0')
                                sprintf(work, "IDAM%s%s", request_block.api_delim, environment.server_this);
                            else
                                sprintf(work, "IDAM%s%s", environment.api_delim, environment.server_this);

                            if((token=strstr(request_block.source, work)) != NULL) {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                             "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                                break;
                            }

            // Check string length compatibility

                            if(strlen(request_block.source) >= (STRING_LENGTH-1 - strlen(environment.server_proxy) - 4+strlen(request_block.api_delim))) {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                             "PROXY redirection: The source argument string is too long!");
                                break;
                            }

            // Prepend the redirection IDAM server details

                            if(request_block.api_delim[0] != '\0')
                                sprintf(work, "IDAM%s%s/%s", request_block.api_delim, environment.server_proxy, request_block.source);
                            else
                                sprintf(work, "IDAM%s%s/%s", environment.api_delim, environment.server_proxy, request_block.source);

                            strcpy(request_block.source, work);
                            //strcpy(request_block.server, environment.server_proxy);

                            if(debugon) {
                                IDAM_LOG(LOG_DEBUG, "PROXY Redirection to %s\n", environment.server_proxy);
                                IDAM_LOG(LOG_DEBUG, "source: %s\n", request_block.source);
                                //IDAM_LOG(LOG_DEBUG, "server: %s\n", request_block.server);
                            }

                        }
#endif
#endif

//----------------------------------------------------------------------
// Write to the Access Log

#ifdef ACCESSLOG
            idamAccessLog(TRUE, client_block, request_block, server_block, data_block);
#endif

#ifdef PERFORMANCETEST
            serverPerformance.npoints = 4;
            rc = gettimeofday(&(serverPerformance.tv_end[2]), NULL);
            strcpy(serverPerformance.label[3],"#3: Decide on Plugin");
            rc = gettimeofday(&(serverPerformance.tv_start[3]), NULL);
#endif

//----------------------------------------------------------------------
// Initialise Data Structures

            initDataSource(&data_source);
            initSignalDesc(&signal_desc);
            initSignal(&signal_rec);

            err = 0;


//----------------------------------------------------------------------------------------------
// If this is a PUT request then receive the putData structure

#ifndef FATCLIENT    // <========================== Client Server Code Only

            initIdamPutDataBlockList((PUTDATA_BLOCK_LIST*) &(request_block.putDataBlockList));

            if (request_block.put) {

                protocol_id = PROTOCOL_PUTDATA_BLOCK_LIST;

                if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, &(request_block.putDataBlockList))) !=
                    0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Receiving putData Block List\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                 "Protocol 1 Error (Receiving Client putDataBlockList)");
                    break;
                }

                IDAM_LOG(LOG_DEBUG, "putData Block List Received\n");
                IDAM_LOGF(LOG_DEBUG, "Number of PutData Blocks: %d\n", request_block.putDataBlockList.blockCount);
            }

// Flush (mark as at EOF) the input socket buffer: no more data should be read from this point

            xdrrec_eof(serverInput);

            IDAM_LOG(LOG_DEBUG, "idamServer: ****** Incoming tcp packet received without error. Accessing data.\n");

#endif

//----------------------------------------------------------------------------------------------
// Decode the API Arguments: determine appropriate data plug-in to use
// Decide on Authentication procedure

#ifdef FATCLIENT
            protocolVersion = serverVersion;
#endif

            if (protocolVersion >= 6) {
                if ((err = idamServerPlugin(&request_block, &data_source, &signal_desc, &pluginList)) != 0) break;
            } else {
                if ((err = idamServerLegacyPlugin(&request_block, &data_source, &signal_desc)) != 0) break;
            }

#ifdef PERFORMANCETEST
            rc = gettimeofday(&(serverPerformance.tv_end[3]), NULL);

            serverPerformance2.npoints = 1;
            strcpy(serverPerformance2.label[0],"#0: Gap - Connect to SQL Database");
            rc = gettimeofday(&(serverPerformance2.tv_start[0]), NULL);
#endif

//------------------------------------------------------------------------------------------------
// Identify the Signal Required from the Database if a Generic Signal Requested
// or if a name mapping (alternative signal/source) is requested by the client
//
// ??? Meta data when an alternative source is requested ???
//------------------------------------------------------------------------------------------------
// Connect to the Database

//#ifdef GENERIC_ENABLE	// <========================== Generic Signal Name Lookup via SQL Database is Enabled
#ifndef NOTGENERICENABLED
            if (request_block.request == REQUEST_READ_GENERIC || (client_block.clientFlags & CLIENTFLAG_ALTDATA)) {
                if (DBConnect == NULL) {
                    if (!(DBConnect = (PGconn*) startSQL())) {
                        if (DBConnect != NULL) PQfinish(DBConnect);
                        err = 777;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                     "Unable to Connect to the SQL Database Server");
                        break;
                    }
                }
                IDAM_LOG(LOG_DEBUG, "IdamServer Connected to SQL Database Server\n");
            }
#endif

#ifdef PERFORMANCETEST
            serverPerformance.npoints = 5;
            strcpy(serverPerformance.label[4],"#4: Read Data from Plugin");
            rc = gettimeofday(&(serverPerformance.tv_start[4]), NULL);

            rc = gettimeofday(&(serverPerformance2.tv_end[0]), NULL);
#endif

//------------------------------------------------------------------------------------------------
// Query the Database: Internal or External Data Sources
// Read the Data or Create the Composite/Derived Data
// Apply XML Actions to Data

            depth = 0;

            err = idamserverGetData(DBConnect, &depth, request_block, client_block, &data_block, &data_source,
                                    &signal_rec, &signal_desc, &actions_desc, &actions_sig);

            if (DBConnect == NULL && gDBConnect != NULL) {
                DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData
                gDBConnect = NULL;
            }

            IDAM_LOG(LOG_DEBUG,
                    "======================== ******************** ==========================================\n");
            IDAM_LOGF(LOG_DEBUG, "Archive      : %s \n", data_source.archive);
            IDAM_LOGF(LOG_DEBUG, "Device Name  : %s \n", data_source.device_name);
            IDAM_LOGF(LOG_DEBUG, "Signal Name  : %s \n", signal_desc.signal_name);
            IDAM_LOGF(LOG_DEBUG, "File Path    : %s \n", data_source.path);
            IDAM_LOGF(LOG_DEBUG, "File Name    : %s \n", data_source.filename);
            IDAM_LOGF(LOG_DEBUG, "Pulse Number : %d \n", data_source.exp_number);
            IDAM_LOGF(LOG_DEBUG, "Pass Number  : %d \n", data_source.pass);
            IDAM_LOGF(LOG_DEBUG, "Recursive #  : %d \n", depth);
            printRequestBlock(request_block);
            printDataSource(data_source);
            printSignal(signal_rec);
            printSignalDesc(signal_desc);
            printDataBlock(data_block);
            printIdamErrorStack(idamerrorstack);
            IDAM_LOG(LOG_DEBUG,
                    "======================== ******************** ==========================================\n");
            if (err != 0) break;


#ifdef PERFORMANCETEST
            rc = gettimeofday(&(serverPerformance.tv_end[4]), NULL);
            serverPerformance.npoints = 6;
            strcpy(serverPerformance.label[5],"#5: Server Side Processing");
            rc = gettimeofday(&(serverPerformance.tv_start[5]), NULL);
#endif

//------------------------------------------------------------------------------------------------
// Server-Side Data Processing

            if (client_block.get_dimdble || client_block.get_timedble || client_block.get_scalar) {
                if ((rc = serverProcessing(client_block, &data_block)) != 0) {
                    err = 779;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Server-Side Processing Error");
                    break;
                }
            }

#ifdef PERFORMANCETEST
            rc = gettimeofday(&(serverPerformance.tv_end[5]), NULL);
            serverPerformance.npoints = 7;
            strcpy(serverPerformance.label[6],"#6: Read Configuration Data");
            rc = gettimeofday(&(serverPerformance.tv_start[6]), NULL);
#endif

//------------------------------------------------------------------------------------------------
// Read Additional Meta Data

//#ifdef GENERIC_ENABLE
#ifndef NOTGENERICENABLED
            if (client_block.get_meta && request_block.request == REQUEST_READ_GENERIC) {

                if ((rc = sqlSystemConfig(DBConnect, data_source.config_id, &system_config)) != 1) {
                    err = 780;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                 "Error Retrieving System Configuration Data");
                    break;
                } else {
                    printSystemConfig(system_config);
                }

                if ((rc = sqlDataSystem(DBConnect, system_config.system_id, &data_system)) != 1) {
                    err = 781;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                 "Error Retrieving Data System Information");
                    break;
                } else {
                    printDataSystem(data_system);
                }
            }

#ifdef PERFORMANCETEST
            rc = gettimeofday(&serverPerformance.tv_end[6], NULL);
            serverPerformance2.npoints = 2;
            strcpy(serverPerformance2.label[1],"#1: Gap - Error Trap Transition");
            rc = gettimeofday(&(serverPerformance2.tv_start[1]), NULL);
#endif

#endif

//----------------------------------------------------------------------------
// Check the Client can receive the data type: Version dependent
// Otherwise inform the client via the server state block

#ifndef FATCLIENT

            if (protocolVersion < 6 && data_block.data_type == TYPE_STRING) data_block.data_type = TYPE_CHAR;

            if (data_block.data_n > 0 &&
                (protocolVersionTypeTest(protocolVersion, data_block.data_type) ||
                 protocolVersionTypeTest(protocolVersion, data_block.error_type))) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "The Data has a type that cannot be passed to the Client: A newer client library version is required.");
                break;
            }


            if (data_block.rank > 0) {
                int i;
                DIMS dim;
                for (i = 0; i < data_block.rank; i++) {
                    dim = data_block.dims[i];
                    if (protocolVersionTypeTest(protocolVersion, dim.data_type) ||
                        protocolVersionTypeTest(protocolVersion, dim.error_type)) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                     "A Coordinate Data has a numerical type that cannot be passed to the Client: A newer client library version is required.");
                        break;
                    }
                }
            }

#endif


//----------------------------------------------------------------------------
// End of Error Trap #1
// If an error has occued within this trap, then a problem occured accessing data
// The server block should be returned with the error stack

        } while (0);

        IDAM_LOGF(LOG_DEBUG, "Leaving Error Trap #1 Loop: %d [%d]\n", err, fatal);

        int trap1Err = err;

//----------------------------------------------------------------------------
// Start of Error Trap Loop #2
// Communication with the client

        do {

            IDAM_LOG(LOG_DEBUG, "Start of Server Error Trap #2 Loop\n");

#ifdef PERFORMANCETEST
            serverPerformance.npoints = 8;
            strcpy(serverPerformance.label[7],"#7: Return Status to Client");
            rc = gettimeofday(&(serverPerformance.tv_start[7]), NULL);

            rc = gettimeofday(&(serverPerformance2.tv_end[1]), NULL);
#endif


//----------------------------------------------------------------------------
// Gather Server Error State

            concatIdamError(idamerrorstack,
                            &server_block.idamerrorstack);        // Update Server State with Error Stack
            closeIdamError(&idamerrorstack);

// <========================== Client Server Code Only
#ifndef FATCLIENT

#ifndef ERRORSTACK
            if(server_block.idamerrorstack.nerrors > 0) {
                server_block.error = server_block.idamerrorstack.idamerror[0].code;
                strcpy(server_block.msg,server_block.idamerrorstack.idamerror[0].msg);
            }
#endif

//------------------------------------------------------------------------------------------------
// How much data to be sent?

            // server_block.totalDataBlockSize = countDataBlockSize(&data_block, &client_block);
            totalDataBlockSize = countDataBlockSize(&data_block, &client_block);

//------------------------------------------------------------------------------------------------
// Send permission for client to cache returned data

            // server_block.cachePermission = cachePermission;

            printServerBlock(server_block);

//------------------------------------------------------------------------------------------------
// Send the server block and all data in a single (minimal number) tcp packet

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &server_block)) != 0) {
                IDAM_LOG(LOG_DEBUG, "Problem Sending Server Data Block #2\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "Protocol 11 Error (Sending Server Block #2)");
                break;
            }

            IDAM_LOG(LOG_DEBUG, "Server Block Sent to Client without error\n");

#endif			// <========================== End of Client Server Code      

            if (server_block.idamerrorstack.nerrors > 0)
                err = server_block.idamerrorstack.idamerror[0].code;
            else
                err = trap1Err;

            if (err != 0) {
                IDAM_LOG(LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop\n");

#ifndef FATCLIENT

                // Send the Server Block and byepass sending data (there are none!)

                if (!(rc = xdrrec_endofrecord(serverOutput, 1))) {
                    err = PROTOCOL_ERROR_7;
                    IDAM_LOG(LOG_DEBUG, "Problem Sending Server Data Block #2\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Server Block)");
                    break;
                }
#endif

                break;
            }

#ifdef PERFORMANCETEST
            rc = gettimeofday(&(serverPerformance.tv_end[7]), NULL);
            serverPerformance.npoints = 9;
            strcpy(serverPerformance.label[8],"#8: Return Meta Data to Client");
            rc = gettimeofday(&(serverPerformance.tv_start[8]), NULL);
#endif

//----------------------------------------------------------------------------
// Return Database Meta Data if User Requests it

// <========================== Client Server Code Only
#ifndef FATCLIENT

            if (client_block.get_meta) {

                totalDataBlockSize +=
                        sizeof(DATA_SYSTEM) + sizeof(SYSTEM_CONFIG) + sizeof(DATA_SOURCE) + sizeof(SIGNAL) +
                        sizeof(SIGNAL_DESC);

//----------------------------------------------------------------------------
// Send the Data System Structure

                protocol_id = PROTOCOL_DATA_SYSTEM;

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &data_system)) != 0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Sending Data System Structure\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 4 Error");
                    break;
                }

//----------------------------------------------------------------------------
// Send the System Configuration Structure

                protocol_id = PROTOCOL_SYSTEM_CONFIG;

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &system_config)) != 0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Sending System Configuration Structure\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error");
                    break;
                }

//----------------------------------------------------------------------------
// Send the Data Source Structure

                protocol_id = PROTOCOL_DATA_SOURCE;

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &data_source)) != 0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Sending Data Source Structure\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 6 Error");
                    break;
                }

//----------------------------------------------------------------------------
// Send the Signal Structure

                protocol_id = PROTOCOL_SIGNAL;

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &signal_rec)) != 0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Sending Signal Structure\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 7 Error");
                    break;
                }

//----------------------------------------------------------------------------
// Send the Signal Description Structure

                protocol_id = PROTOCOL_SIGNAL_DESC;

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &signal_desc)) != 0) {
                    IDAM_LOG(LOG_DEBUG, "Problem Sending Signal Description Structure\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 8 Error");
                    break;
                }

            } // End of Database Meta Data


#ifdef PERFORMANCETEST
            rc = gettimeofday(&(serverPerformance.tv_end[8]), NULL);
            serverPerformance.npoints = 10;
            strcpy(serverPerformance.label[9],"#9: Return Data to Client");
            rc = gettimeofday(&(serverPerformance.tv_start[9]), NULL);
#endif

//----------------------------------------------------------------------------
// Send the Data

            printDataBlock(data_block);
            IDAM_LOG(LOG_DEBUG, "Sending Data Block Structure to Client\n");

            protocol_id = PROTOCOL_DATA_BLOCK;

            if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &data_block)) != 0) {
                IDAM_LOG(LOG_DEBUG, "Problem Sending Data Structure\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 2 Error");
                break;
            }

            IDAM_LOG(LOG_DEBUG, "Data Block Sent to Client\n");

#ifdef PERFORMANCETEST
            rc = gettimeofday(&serverPerformance.tv_end[9], NULL);
            serverPerformance.npoints = 11;
            strcpy(serverPerformance.label[10],"#10: Return Hierarchical Data to Client");
            rc = gettimeofday(&(serverPerformance.tv_start[10]), NULL);
#endif

//----------------------------------------------------------------------------
// Send the data in a single full TCP packet


// ******* is this an extra XDR EOF ?????

#ifndef FATCLIENT
            if (!(rc = xdrrec_endofrecord(serverOutput, 1))) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Server Block)");
                break;
            }
#endif

//------------------------------------------------------------------------------
// Legacy Hierarchical Data Structures

            if (protocolVersion < 8 && data_block.data_type == TYPE_COMPOUND &&
                data_block.opaque_type != OPAQUE_TYPE_UNKNOWN) {
                if (data_block.opaque_type == OPAQUE_TYPE_XML_DOCUMENT) {
                    protocol_id = PROTOCOL_META;
                } else {
                    if (data_block.opaque_type == OPAQUE_TYPE_STRUCTURES ||
                        data_block.opaque_type == OPAQUE_TYPE_XDRFILE) {
                        protocol_id = PROTOCOL_STRUCTURES;
                    } else {
                        protocol_id = PROTOCOL_EFIT;
                    }
                }

                IDAM_LOG(LOG_DEBUG, "Sending Hierarchical Data Structure to Client\n");

                if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, &data_block)) != 0) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                 "Server Side Protocol Error (Opaque Structure Type)");
                    break;
                }

                IDAM_LOG(LOG_DEBUG, "Hierarchical Data Structure sent to Client\n");
            }

#ifdef PERFORMANCETEST
            rc = gettimeofday(&serverPerformance.tv_end[10], NULL);

            serverPerformance2.npoints = 3;
            strcpy(serverPerformance2.label[2],"#2: Gap - Final Protocol");
            rc = gettimeofday(&(serverPerformance2.tv_start[2]), NULL);
#endif

#else			// <========================== End of Client Server Code

            //------------------------------------------------------------------------------
            // Hierarchical Data Structures: Transform into a Data Tree via XDR IO streams to/from a temporary file (inefficient!)

            // Avoid multiple heap malloc and file writing by creating tree node directly

            //-----------------------------------------------------------------------------------
            // If user changes property ....
            // Send temp file to client: files saved to specified temporary or scratch directory
            // File must include a date-time stamp (prevent users from tampering: users can keep copy but only valid
            // for a short period of time, e.g. 24Hrs.)
            // Enable the client to read previous files if date-stamp is current - check made when file opened for read.
            // Client manages a log of available files: records signal source argument pairs.
            // Client deletes stale files automatically on startup.
            //----------------------------------------------------------------------------------

#ifdef GENERALSTRUCTS
#ifdef FATCLIENT

                    if (data_block.opaque_type == OPAQUE_TYPE_STRUCTURES) {

            // Create an output XDR stream

                        FILE * xdrfile;
                        char tempFile[MAXPATH];
                        char * env;
                        if ((env = getenv("IDAM_WORK_DIR")) != NULL) {
                            sprintf(tempFile, "%s/idamXDRXXXXXX", env);
                        } else {
                            strcpy(tempFile, "/tmp/idamXDRXXXXXX");
                        }

                        DATA_BLOCK DBlockCopy = data_block;        // Risk: Potential double free of heap

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 995;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamServer", err,
                                         " Unable to Obtain a Temporary File Name");
                            err = 995;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, tempFile);
                            break;
                        }
                        if ((xdrfile = fopen(tempFile, "w")) == NULL) {
                            err = 999;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamServer", err,
                                         " Unable to Open a Temporary XDR File for Writing");
                            break;
                        }

                        xdrstdio_create(serverOutput, xdrfile, XDR_ENCODE);

            // Write data to the temporary file

                        protocol_id = PROTOCOL_STRUCTURES;
                        err = protocolXML(serverOutput, protocol_id, XDR_SEND, NULL, &data_block);

            // Close the stream and file

                        fflush(xdrfile);
                        fclose(xdrfile);

            // Free Heap

                        freeReducedDataBlock(&data_block);        // Avoid double free in fatclient
                        data_block = DBlockCopy;

            // Create an input XDR stream

                        if ((xdrfile = fopen(tempFile, "r")) == NULL) {
                            err = 999;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamServer", err,
                                         " Unable to Open a Temporary XDR File for Reading");
                            break;
                        }

                        xdrstdio_create(serverInput, xdrfile, XDR_DECODE);

            // Read data from the temporary file

                        protocol_id = PROTOCOL_STRUCTURES;
                        err = protocolXML(serverInput, protocol_id, XDR_RECEIVE, NULL, &data_block);

            // Close the stream and file

                        fclose(xdrfile);

            // Remove the Temporary File

                        remove(tempFile);

                    }

#endif
#endif

            //------------------------------------------------------------------------------

#ifndef NOTGENERICENABLED
                    if (client_block.get_meta && request_block.request == REQUEST_READ_GENERIC) {

                        data_block.data_system = data_block0->data_system;    // Allocations made in Client API when FAT
                        data_block.system_config = data_block0->system_config;
                        data_block.data_source = data_block0->data_source;
                        data_block.signal_rec = data_block0->signal_rec;
                        data_block.signal_desc = data_block0->signal_desc;

                        initDataSystem(data_block.data_system);
                        initSystemConfig(data_block.system_config);
                        initDataSource(data_block.data_source);
                        initSignal(data_block.signal_rec);
                        initSignalDesc(data_block.signal_desc);

                        *data_block.data_system = data_system;
                        *data_block.system_config = system_config;
                        *data_block.data_source = data_source;
                        *data_block.signal_rec = signal_rec;
                        *data_block.signal_desc = signal_desc;

                    }

#endif

            //----------------------------------------------------------------------------
            // Free Name Value pair

                    freeNameValueList(&request_block.nameValueList);

#endif			// <========================== End of Fat Client Code  

//----------------------------------------------------------------------------
// End of Error Trap #2

        } while (0);

        IDAM_LOG(LOG_DEBUG, "Data structures sent to client\n");
        IDAM_LOGF(LOG_DEBUG, "Leaving Error Trap #2 Loop: %d [%d]\n", err, fatal);

//----------------------------------------------------------------------
// Complete & Write the Access Log Record

#ifdef ACCESSLOG
#ifdef FATCLIENT
        server_accout = dbgout;
#endif
        idamAccessLog(FALSE, client_block, request_block, server_block, data_block);
#endif

//----------------------------------------------------------------------------
// Server Shutdown ? Next Instruction from Client
//
// Protocols:   13 => Die
//		14 => Sleep
//		15 => Wakeup

// <========================== Client Server Code Only

#ifndef FATCLIENT

        err = 0;
        protocol_id = PROTOCOL_NEXT_PROTOCOL;
        next_protocol = PROTOCOL_SLEEP;
        IDAM_LOGF(LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);

#ifdef PERFORMANCETEST
        rc = gettimeofday(&serverPerformance2.tv_end[2], NULL);

        serverPerformance2.npoints = 4;
        strcpy(serverPerformance2.label[3],"#3: Gap - House keeping");
        rc = gettimeofday(&(serverPerformance2.tv_start[3]), NULL);
#endif

//----------------------------------------------------------------------------
// Free Data Block Heap Memory

        IDAM_LOG(LOG_DEBUG, "freeDataBlock\n");
        freeDataBlock(&data_block);

        IDAM_LOG(LOG_DEBUG, "freeActions\n");
        freeActions(&actions_desc);

        IDAM_LOG(LOG_DEBUG, "freeActions\n");
        freeActions(&actions_sig);

#ifdef PERFORMANCETEST
        rc = gettimeofday(&serverPerformance.tv_end[1], NULL);
        rc = gettimeofday(&(serverPerformance2.tv_end[3]), NULL);
        if(debugon) {
            printPerformance(dbgout,serverPerformance);
            printPerformance(dbgout,serverPerformance2);
        }
#endif

//----------------------------------------------------------------------------
// Free Name Value pair

        freeNameValueList(&request_block.nameValueList);

//----------------------------------------------------------------------------
// Free PutData Blocks

        freeIdamServerPutDataBlockList(&request_block.putDataBlockList);

//----------------------------------------------------------------------------
// Write the Error Log Record & Free Error Stack Heap

        IDAM_LOG(LOG_DEBUG, "concatIdamError\n");
        concatIdamError(idamerrorstack, &server_block.idamerrorstack);        // Update Server State with Error Stack

        IDAM_LOG(LOG_DEBUG, "closeIdamError\n");
        closeIdamError(&idamerrorstack);

        IDAM_LOG(LOG_DEBUG, "idamErrorLog\n");
        idamErrorLog(client_block, request_block, server_block.idamerrorstack);

        IDAM_LOG(LOG_DEBUG, "closeIdamError\n");
        closeIdamError(&server_block.idamerrorstack);

        IDAM_LOG(LOG_DEBUG, "initServerBlock\n");
        initServerBlock(&server_block, serverVersion);

        IDAM_LOG(LOG_DEBUG, "At End of Error Trap\n");

//----------------------------------------------------------------------------
// Server Wait Loop

        //} while(err == 0 && next_protocol == PROTOCOL_SLEEP && sleepServer() );

    } while (err == 0 && next_protocol == PROTOCOL_SLEEP && !fatal);

//----------------------------------------------------------------------------
// Server Destruct.....

    SERVERCLOSEDOWN:

    IDAM_LOG(LOG_DEBUG, "Server Shuting Down\n");
    if (server_tot_block_time > 1000 * server_timeout)
        IDAM_LOGF(LOG_DEBUG, "Server Timeout after %d secs\n", server_timeout);

//----------------------------------------------------------------------------
// Write the Error Log Record & Free Error Stack Heap

    idamErrorLog(client_block, request_block, idamerrorstack);
    closeIdamError(&idamerrorstack);

//----------------------------------------------------------------------------
// Free Data Block Heap Memory in case by-passed

    freeDataBlock(&data_block);

//----------------------------------------------------------------------------
// Free Structure Definition List (don't free the structure as stack variable)

#ifdef GENERALSTRUCTS
    freeUserDefinedTypeList(&parseduserdefinedtypelist);
#endif

//----------------------------------------------------------------------------
// Free Plugin List and Close all open library entries

    freePluginList(&pluginList);

//----------------------------------------------------------------------------
// Close the Database Connection

//#ifdef GENERIC_ENABLE
#ifndef NOTGENERICENABLED
    if (DBConnect != NULL) PQfinish(DBConnect);
#endif

//----------------------------------------------------------------------------
// Close the Logs

    rc = fflush(NULL);
    if (server_accout != NULL) fclose(server_accout);

    idamCloseLogging();

//----------------------------------------------------------------------------
// Close Open Data Source Files (legacy: not plugin based)

#ifdef FILELISTTEST
    closeIdamFiles(&idamfilelist);
#endif


//----------------------------------------------------------------------------
// Close the Socket Connections to Other Data Servers

    closeServerSockets(&server_socketlist);

//----------------------------------------------------------------------------
// Wait for client to receive returned server state

//sleep(1);		// 1 Sec

    return 0;

#else			// <========================== End of Client Server Code


    //----------------------------------------------------------------------------
    // Free Actions Heap

        freeActions(&actions_desc);
        freeActions(&actions_sig);

    //----------------------------------------------------------------------------

        concatIdamError(idamerrorstack, &server_block.idamerrorstack);        // Update Server State with Global Error Stack
        closeIdamError(&idamerrorstack);

        *server_block0 = server_block;
        *data_block0 = data_block;

        printDataBlock(*data_block0);

        return 0;

#endif			// <========================== End of Fat Client Code         

}
