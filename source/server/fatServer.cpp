#include <stdio.h>
#include <rpc/rpc.h>
#include <assert.h>
#include <errno.h>

#include <clientserver/copyStructs.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/manageSockets.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/protocolXML.h>
#include <clientserver/xdrlib.h>
#include <clientserver/mkstemp.h>
#include <logging/accessLog.h>
#include <server/serverPlugin.h>
#include <structures/parseIncludeFile.h>
#include <structures/struct.h>

#include "getServerEnvironment.h"
#include "getServerEnvironment.h"
#include "makeServerRequestBlock.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "sqllib.h"

#ifdef NONETCDFPLUGIN
void ncclose(int fh) {
}
#endif

unsigned int totalDataBlockSize = 0;

int server_tot_block_time = 0;

int server_timeout = TIMEOUT;

static PLUGINLIST pluginList;      // List of all data reader plugins (internal and external shared libraries)
ENVIRONMENT environment;    // Holds local environment variable values

static USERDEFINEDTYPELIST* userdefinedtypelist = nullptr;
static LOGMALLOCLIST* logmalloclist = nullptr;

PGconn* DBConnect = nullptr;
PGconn* gDBConnect = nullptr;

unsigned int XDRstdioFlag = 1;
int altRank = 0;
unsigned int clientFlags = 0;
NTREE* fullNTree = nullptr;

int malloc_source = MALLOCSOURCENONE;
USERDEFINEDTYPELIST parseduserdefinedtypelist;
unsigned int privateFlags = 0;

int serverVersion = 7;
static int protocolVersion = 7;

SOCKETLIST socket_list;

typedef struct MetadataBlock {
    DATA_SOURCE data_source;
    SIGNAL signal_rec;
    SIGNAL_DESC signal_desc;
    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
} METADATA_BLOCK;

#ifdef FATCLIENT
extern "C" {

void setUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist_in)
{
    userdefinedtypelist = userdefinedtypelist_in;
}

void setLogMallocList(LOGMALLOCLIST* logmalloclist_in)
{
    logmalloclist = logmalloclist_in;
}

}
#endif

static int startupFatServer(SERVER_BLOCK* server_block);

static int doFatServerClosedown(SERVER_BLOCK* server_block, DATA_BLOCK* data_block, ACTIONS* actions_desc,
                                ACTIONS* actions_sig, DATA_BLOCK* data_block0);

static int handleRequestFat(REQUEST_BLOCK* request_block, REQUEST_BLOCK* request_block0, CLIENT_BLOCK* client_block,
                            SERVER_BLOCK* server_block, METADATA_BLOCK* metadata_block, DATA_BLOCK* data_block,
                            ACTIONS* actions_desc, ACTIONS* actions_sig);

static int fatClientReturn(SERVER_BLOCK* server_block, DATA_BLOCK* data_block, DATA_BLOCK* data_block0,
                           REQUEST_BLOCK* request_block, CLIENT_BLOCK* client_block, METADATA_BLOCK* metadata_block);

//--------------------------------------------------------------------------------------
// Server Entry point

int
fatServer(CLIENT_BLOCK client_block, SERVER_BLOCK* server_block, REQUEST_BLOCK* request_block0, DATA_BLOCK* data_block0)
{
    assert(data_block0 != nullptr);

    METADATA_BLOCK metadata_block;
    memset(&metadata_block, '\0', sizeof(METADATA_BLOCK));

    DATA_BLOCK data_block;
    REQUEST_BLOCK request_block;

    ACTIONS actions_desc;
    ACTIONS actions_sig;

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    initServerBlock(server_block, serverVersion);
    initDataBlock(&data_block);
    initActions(&actions_desc);        // There may be a Sequence of Actions to Apply
    initActions(&actions_sig);

    getInitialUserDefinedTypeList(&userdefinedtypelist);
    parseduserdefinedtypelist = *userdefinedtypelist;
    //printUserDefinedTypeList(*userdefinedtypelist);

    logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
    initLogMallocList(logmalloclist);

    int err = startupFatServer(server_block);
    if (err != 0) {
        return err;
    }

    copyUserDefinedTypeList(&userdefinedtypelist);

    err = handleRequestFat(&request_block, request_block0, &client_block, server_block, &metadata_block, &data_block,
                           &actions_desc, &actions_sig);
    if (err != 0) {
        return err;
    }

    err = fatClientReturn(server_block, &data_block, data_block0, &request_block, &client_block, &metadata_block);
    if (err != 0) {
        return err;
    }

    idamAccessLog(FALSE, client_block, request_block, *server_block, &pluginList, getIdamServerEnvironment());

    err = doFatServerClosedown(server_block, &data_block, &actions_desc, &actions_sig, data_block0);

    freeUserDefinedTypeList(userdefinedtypelist);
    free(userdefinedtypelist);
    userdefinedtypelist = nullptr;

    //freeMallocLogList(logmalloclist);
    //free(logmalloclist);

    return err;
}

int fatClientReturn(SERVER_BLOCK* server_block, DATA_BLOCK* data_block, DATA_BLOCK* data_block0,
                    REQUEST_BLOCK* request_block,
                    CLIENT_BLOCK* client_block, METADATA_BLOCK* metadata_block)
{
    //----------------------------------------------------------------------------
    // Gather Server Error State

    // Update Server State with Error Stack
    concatIdamError(&server_block->idamerrorstack);
    closeIdamError();

    int err = 0;

    if (server_block->idamerrorstack.nerrors > 0) {
        err = server_block->idamerrorstack.idamerror[0].code;
        return err;
    }

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

    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {

        // Create an output XDR stream

        FILE* xdrfile;
        char tempFile[MAXPATH];
        char* env;
        if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
            sprintf(tempFile, "%s/idamXDRXXXXXX", env);
        } else {
            strcpy(tempFile, "/tmp/idamXDRXXXXXX");
        }

        DATA_BLOCK data_block_copy = *data_block;

        errno = 0;
        if (mkstemp(tempFile) < 0 || errno != 0) {
            THROW_ERROR(995, "Unable to Obtain a Temporary File Name");
        }
        if ((xdrfile = fopen(tempFile, "wb")) == nullptr) {
            THROW_ERROR(999, "Unable to Open a Temporary XDR File for Writing");
        }

        XDR xdrServerOutput;
        xdrstdio_create(&xdrServerOutput, xdrfile, XDR_ENCODE);

        // Write data to the temporary file

        int protocol_id = PROTOCOL_STRUCTURES;
        protocolXML(&xdrServerOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist, data_block,
                    protocolVersion);

        // Close the stream and file

        xdr_destroy(&xdrServerOutput);
        fclose(xdrfile);

        // Free Heap

        freeReducedDataBlock(data_block);
        *data_block = data_block_copy;

        // Create an input XDR stream

        if ((xdrfile = fopen(tempFile, "rb")) == nullptr) {
            THROW_ERROR(999, "Unable to Open a Temporary XDR File for Reading");
        }

        XDR xdrServerInput;
        xdrstdio_create(&xdrServerInput, xdrfile, XDR_DECODE);

        // Read data from the temporary file

        protocol_id = PROTOCOL_STRUCTURES;
        err = protocolXML(&xdrServerInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                          data_block, protocolVersion);

        // Close the stream and file

        xdr_destroy(&xdrServerInput);
        fclose(xdrfile);

        // Remove the Temporary File

        remove(tempFile);
    }

    //------------------------------------------------------------------------------

#ifndef NOTGENERICENABLED
    if (client_block->get_meta && request_block->request == REQUEST_READ_GENERIC) {
        data_block->data_system = data_block0->data_system;    // Allocations made in Client API when FAT
        data_block->system_config = data_block0->system_config;
        data_block->data_source = data_block0->data_source;
        data_block->signal_rec = data_block0->signal_rec;
        data_block->signal_desc = data_block0->signal_desc;

        initDataSystem(data_block->data_system);
        initSystemConfig(data_block->system_config);
        initDataSource(data_block->data_source);
        initSignal(data_block->signal_rec);
        initSignalDesc(data_block->signal_desc);

        *data_block->data_system = metadata_block->data_system;
        *data_block->system_config = metadata_block->system_config;
        *data_block->data_source = metadata_block->data_source;
        *data_block->signal_rec = metadata_block->signal_rec;
        *data_block->signal_desc = metadata_block->signal_desc;
    }
# endif

    //----------------------------------------------------------------------------
    // Free Name Value pair

    freeNameValueList(&request_block->nameValueList);

    return err;
}

int handleRequestFat(REQUEST_BLOCK* request_block, REQUEST_BLOCK* request_block0, CLIENT_BLOCK* client_block,
                     SERVER_BLOCK* server_block, METADATA_BLOCK* metadata_block, DATA_BLOCK* data_block,
                     ACTIONS* actions_desc, ACTIONS* actions_sig)
{
    UDA_LOG(UDA_LOG_DEBUG, "IdamServer: Start of Server Error Trap #1 Loop\n");

    copyRequestBlock(request_block, *request_block0);

    int err = 0;

    printClientBlock(*client_block);
    printServerBlock(*server_block);
    printRequestBlock(*request_block);

    char work[1024];
    if (request_block->api_delim[0] != '\0') {
        sprintf(work, "UDA%s", request_block->api_delim);
    } else {
        sprintf(work, "UDA%s", environment.api_delim);
    }

    //----------------------------------------------------------------------
    // Initialise Data Structures

    initDataSource(&metadata_block->data_source);
    initSignalDesc(&metadata_block->signal_desc);
    initSignal(&metadata_block->signal_rec);

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data plug-in to use
    // Decide on Authentication procedure

    protocolVersion = serverVersion;

    if (protocolVersion >= 6) {
        if ((err = idamServerPlugin(request_block, &metadata_block->data_source, &metadata_block->signal_desc,
                                    &pluginList, getIdamServerEnvironment())) != 0) {
            return err;
        }
    } else {
        if ((err = idamServerLegacyPlugin(request_block, &metadata_block->data_source, &metadata_block->signal_desc)) !=
            0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // or if a name mapping (alternative signal/source) is requested by the client
    //
    // ??? Meta data when an alternative source is requested ???
    //------------------------------------------------------------------------------------------------
    // Connect to the Database

#ifndef NOTGENERICENABLED
    if (request_block->request == REQUEST_READ_GENERIC || (client_block->clientFlags & CLIENTFLAG_ALTDATA)) {
        if (DBConnect == nullptr) {
            if (!(DBConnect = startSQL(getIdamServerEnvironment()))) {
                if (DBConnect != nullptr) {
                    PQfinish(DBConnect);
                }
                THROW_ERROR(777, "Unable to Connect to the SQL Database Server");
            }
        }
        UDA_LOG(UDA_LOG_DEBUG, "Connected to SQL Database Server\n");
    }
#endif

    //------------------------------------------------------------------------------------------------
    // Query the Database: Internal or External Data Sources
    // Read the Data or Create the Composite/Derived Data
    // Apply XML Actions to Data

    int depth = 0;

    err = udaGetData(*request_block, *client_block, data_block, &metadata_block->data_source,
                     &metadata_block->signal_rec, &metadata_block->signal_desc, &pluginList, logmalloclist,
                     userdefinedtypelist);

    if (err != 0) {
        return err;
    }

    if (DBConnect == nullptr && gDBConnect != nullptr) {
        DBConnect = gDBConnect;    // Pass back SQL Socket from idamserverGetData
        gDBConnect = nullptr;
    }

    DATA_SOURCE* data_source = &metadata_block->data_source;
    SIGNAL_DESC* signal_desc = &metadata_block->signal_desc;
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source->pass);
    UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : %d \n", depth);
    printRequestBlock(*request_block);
    printDataSource(*data_source);
    printSignal(metadata_block->signal_rec);
    printSignalDesc(*signal_desc);
    printDataBlock(*data_block);
    printIdamErrorStack();
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");

    //------------------------------------------------------------------------------------------------
    // Server-Side Data Processing

    if (client_block->get_dimdble || client_block->get_timedble || client_block->get_scalar) {
        if (serverProcessing(*client_block, data_block) != 0) {
            THROW_ERROR(779, "Server-Side Processing Error");
        }
    }

    //------------------------------------------------------------------------------------------------
    // Read Additional Meta Data

#ifndef NOTGENERICENABLED
    if (client_block->get_meta && request_block->request == REQUEST_READ_GENERIC) {
        if (sqlSystemConfig(DBConnect, data_source->config_id, &metadata_block->system_config) != 1) {
            THROW_ERROR(780, "Error Retrieving System Configuration Data");
        }

        printSystemConfig(metadata_block->system_config);

        if (sqlDataSystem(DBConnect, metadata_block->system_config.system_id, &metadata_block->data_system) != 1) {
            THROW_ERROR(781, "Error Retrieving Data System Information");
        }

        printDataSystem(metadata_block->data_system);
    }
#endif

    //----------------------------------------------------------------------------
    // End of Error Trap #1
    // If an error has occued within this trap, then a problem occured accessing data
    // The server block should be returned with the error stack

    return err;
}

int doFatServerClosedown(SERVER_BLOCK* server_block, DATA_BLOCK* data_block, ACTIONS* actions_desc,
                         ACTIONS* actions_sig, DATA_BLOCK* data_block0)
{
    //----------------------------------------------------------------------------
    // Free Plugin List and Close all open library entries

    // freePluginList(&pluginList);

    //----------------------------------------------------------------------------
    // Free Actions Heap

    freeActions(actions_desc);
    freeActions(actions_sig);

    //----------------------------------------------------------------------------

    concatIdamError(&server_block->idamerrorstack); // Update Server State with Global Error Stack
    closeIdamError();

    *data_block0 = *data_block;

    printDataBlock(*data_block0);

    return 0;
}

int startupFatServer(SERVER_BLOCK* server_block)
{
    static int socket_list_initialised = 0;
    static int plugin_list_initialised = 0;
    //static int fileParsed = 0;

    //-------------------------------------------------------------------------
    // Open and Initialise the Socket List (Once Only)

    if (!socket_list_initialised) {
        initSocketList(&socket_list);
        socket_list_initialised = 1;
    }

    //----------------------------------------------------------------------
    // Initialise General Structure Passing

    getInitialUserDefinedTypeList(&userdefinedtypelist);
    parseduserdefinedtypelist = *userdefinedtypelist;
    printUserDefinedTypeList(*userdefinedtypelist);
    userdefinedtypelist = nullptr;                                     // Startup State


    /*
    // this step needs doing once only - the first time a generalised user defined structure is encountered.
    // For FAT clients use a static state variable to prevent multiple parsing

    if (!fileParsed) {
        fileParsed = 1;
	
        initUserDefinedTypeList(&parseduserdefinedtypelist);
        userdefinedtypelist = &parseduserdefinedtypelist; // Switch before Parsing input file

        char* token = nullptr;
        if ((token = getenv("UDA_SARRAY_CONFIG")) == nullptr) {
            THROW_ERROR(999, "No Environment variable UDA_SARRAY_CONFIG");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Parsing structure definition file: %s\n", token);
        parseIncludeFile(userdefinedtypelist, token); // file containing the SARRAY structure definition
        parseduserdefinedtypelist = *userdefinedtypelist; // Switch back
        printUserDefinedTypeList(parseduserdefinedtypelist);
    }
    */

    //----------------------------------------------------------------------
    // Initialise the Data Reader Plugin list

    if (!plugin_list_initialised) {
        pluginList.count = 0;
        initPluginList(&pluginList, getIdamServerEnvironment());
        plugin_list_initialised = 1;

        UDA_LOG(UDA_LOG_INFO, "List of Plugins available\n");
        int i;
        for (i = 0; i < pluginList.count; i++) {
            UDA_LOG(UDA_LOG_INFO, "[%d] %d %s\n", i, pluginList.plugin[i].request, pluginList.plugin[i].format);
        }
    }

    //----------------------------------------------------------------------------
    // Server Information: Operating System Name - may limit types of data that can be received by the Client

    char* env = nullptr;

    if ((env = getenv("OSTYPE")) != nullptr) {
        strcpy(server_block->OSName, env);
    } else if ((env = getenv("UDA_SERVER_OS")) != nullptr) {
        strcpy(server_block->OSName, env);
    }

    // Server Configuration and Environment DOI

    if ((env = getenv("UDA_SERVER_DOI")) != nullptr) {
        strcpy(server_block->DOI, env);
    }

    return 0;
}
