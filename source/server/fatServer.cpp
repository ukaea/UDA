#include "udaServer.h"

#include <cassert>
#include <cerrno>
#include <cstdio>

#include "clientserver/copyStructs.h"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/manageSockets.h"
#include "clientserver/printStructs.h"
#include "clientserver/protocol.h"
#include "clientserver/protocolXML.h"
#include "clientserver/xdrlib.h"
#include "logging/accessLog.h"
#include "logging/logging.h"
#include "server/serverPlugin.h"
#include "structures/struct.h"

#include "createXDRStream.h"
#include "getServerEnvironment.h"
#include "initPluginList.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "uda/structured.h"

#ifdef NONETCDFPLUGIN
void ncclose(int fh) {}
#endif

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

static uda::plugins::PluginList pluginList; // List of all data reader plugins (internal and external shared libraries)
Environment environment;                    // Holds local environment variable values

static UserDefinedTypeList* user_defined_type_list = nullptr;
static LogMallocList* log_malloc_list = nullptr;

int altRank = 0;
unsigned int clientFlags = 0;

int malloc_source = UDA_MALLOC_SOURCE_NONE;
unsigned int private_flags = 0;

int server_version = 8;
static int protocol_version = 8;

SOCKETLIST socket_list;

struct MetaDataBlock {
    DataSource data_source;
    Signal signal_rec;
    SignalDesc signal_desc;
    SystemConfig system_config;
    DataSystem data_system;
};

#ifdef FATCLIENT
extern "C" {

void setUserDefinedTypeList(UserDefinedTypeList* userdefinedtypelist_in)
{
    user_defined_type_list = userdefinedtypelist_in;
}

void setLogMallocList(LogMallocList* logmalloclist_in)
{
    log_malloc_list = logmalloclist_in;
}
}
#endif

static int startup_fat_server(ServerBlock* server_block, UserDefinedTypeList& parseduserdefinedtypelist);

static int do_fat_server_closedown(ServerBlock* server_block, DataBlockList* data_blocks, Actions* actions_desc,
                                   Actions* actions_sig, DataBlockList* data_blocks0);

static int handle_request_fat(RequestBlock* request_block, RequestBlock* request_block0, ClientBlock* client_block,
                              ServerBlock* server_block, MetaDataBlock* metadata_block, DataBlockList* data_block,
                              Actions* actions_desc, Actions* actions_sig);

static int fat_client_return(ServerBlock* server_block, DataBlockList* data_blocks, DataBlockList* data_blocks0,
                             RequestBlock* request_block, ClientBlock* client_block, MetaDataBlock* metadata_block,
                             LogStructList* log_struct_list, uda::server::IoData* io_data);

//--------------------------------------------------------------------------------------
// Server Entry point

int uda::server::fat_server(ClientBlock client_block, ServerBlock* server_block, RequestBlock* request_block0,
                            DataBlockList* data_blocks0)
{
    assert(data_blocks0 != nullptr);

    MetaDataBlock metadata_block;
    memset(&metadata_block, '\0', sizeof(MetaDataBlock));

    DataBlockList data_blocks;
    RequestBlock request_block;

    Actions actions_desc;
    Actions actions_sig;

    LogStructList log_struct_list;
    init_log_struct_list(&log_struct_list);

    int server_tot_block_time = 0;
    int server_timeout = TIMEOUT; // user specified Server Lifetime

    IoData io_data = {};
    io_data.server_tot_block_time = &server_tot_block_time;
    io_data.server_timeout = &server_timeout;

    static unsigned int total_datablock_size = 0;

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    init_server_block(server_block, server_version);
    init_data_block_list(&data_blocks);
    init_actions(&actions_desc); // There may be a Sequence of Actions to Apply
    init_actions(&actions_sig);

    UserDefinedTypeList parseduserdefinedtypelist;

    get_initial_user_defined_type_list(&user_defined_type_list);
    parseduserdefinedtypelist = *user_defined_type_list;
    // print_user_defined_type_list(*userdefinedtypelist);

    log_malloc_list = (LogMallocList*)malloc(sizeof(LogMallocList));
    init_log_malloc_list(log_malloc_list);

    int err = startup_fat_server(server_block, parseduserdefinedtypelist);
    if (err != 0) {
        return err;
    }

    copy_user_defined_type_list(&user_defined_type_list, &parseduserdefinedtypelist);

    err = handle_request_fat(&request_block, request_block0, &client_block, server_block, &metadata_block, &data_blocks,
                             &actions_desc, &actions_sig);
    if (err != 0) {
        return err;
    }

    err = fat_client_return(server_block, &data_blocks, data_blocks0, &request_block, &client_block, &metadata_block,
                            &log_struct_list, &io_data);
    if (err != 0) {
        return err;
    }

    udaAccessLog(FALSE, client_block, request_block, *server_block, total_datablock_size);

    err = do_fat_server_closedown(server_block, &data_blocks, &actions_desc, &actions_sig, data_blocks0);

    udaFreeUserDefinedTypeList(user_defined_type_list);
    free(user_defined_type_list);
    user_defined_type_list = nullptr;

    // udaFreeMallocLogList(logmalloclist);
    // free(logmalloclist);

    return err;
}

/**
 * Hierarchical Data Structures: Transform into a Data Tree via XDR IO streams to/from a temporary file (inefficient!)
 *
 * Avoid multiple heap malloc and file writing by creating tree node directly.
 *
 * If user changes property ....
 * Send temp file to client: files saved to specified temporary or scratch directory
 * File must include a date-time stamp (prevent users from tampering: users can keep copy but only valid for a short
 * period of time, e.g. 24Hrs.)
 * Enable the client to read previous files if date-stamp is current - check made when file opened for read.
 * Client manages a log of available files: records signal source argument pairs.
 * Client deletes stale files automatically on startup.
 * @return
 */
static int process_hierarchical_data(DataBlock* data_block, LogStructList* log_struct_list,
                                     uda::server::IoData* io_data)
{
    int err = 0;

    // Create an output XDR stream

    FILE* xdrfile;
    char tempFile[MAXPATH];
    char* env;
    if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
        snprintf(tempFile, MAXPATH, "%s/idamXDRXXXXXX", env);
    } else {
        strcpy(tempFile, "/tmp/idamXDRXXXXXX");
    }

    DataBlock data_block_copy = *data_block;

    errno = 0;
    if (mkstemp(tempFile) < 0 || errno != 0) {
        UDA_THROW_ERROR(995, "Unable to Obtain a Temporary File Name");
    }
    if ((xdrfile = fopen(tempFile, "wb")) == nullptr) {
        UDA_THROW_ERROR(999, "Unable to Open a Temporary XDR File for Writing");
    }

    XDR xdr_server_output;
    xdrstdio_create(&xdr_server_output, xdrfile, XDR_ENCODE);

    // Write data to the temporary file

    int protocol_id = UDA_PROTOCOL_STRUCTURES;
    protocol_xml(&xdr_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list, data_block,
                protocol_version, log_struct_list, io_data, private_flags, malloc_source, serverCreateXDRStream);

    // Close the stream and file

    xdr_destroy(&xdr_server_output);
    fclose(xdrfile);

    // Free Heap

    freeReducedDataBlock(data_block);
    *data_block = data_block_copy;

    // Create an input XDR stream

    if ((xdrfile = fopen(tempFile, "rb")) == nullptr) {
        UDA_THROW_ERROR(999, "Unable to Open a Temporary XDR File for Reading");
    }

    XDR xdr_server_input;
    xdrstdio_create(&xdr_server_input, xdrfile, XDR_DECODE);

    // Read data from the temporary file

    protocol_id = UDA_PROTOCOL_STRUCTURES;
    err = protocol_xml(&xdr_server_input, protocol_id, XDR_RECEIVE, nullptr, log_malloc_list, user_defined_type_list,
                      data_block, protocol_version, log_struct_list, io_data, private_flags, malloc_source,
                      serverCreateXDRStream);

    // Close the stream and file

    xdr_destroy(&xdr_server_input);
    fclose(xdrfile);

    // Remove the Temporary File

    remove(tempFile);

    return err;
}

int fat_client_return(ServerBlock* server_block, DataBlockList* data_blocks, DataBlockList* data_blocks0,
                      RequestBlock* request_block, ClientBlock* client_block, MetaDataBlock* metadata_block,
                      LogStructList* log_struct_list, uda::server::IoData* io_data)
{
    //----------------------------------------------------------------------------
    // Gather Server Error State

    // Update Server State with Error Stack
    concat_error(&server_block->idamerrorstack);
    close_error();

    int err = 0;

    if (server_block->idamerrorstack.nerrors > 0) {
        err = server_block->idamerrorstack.idamerror[0].code;
        return err;
    }

    for (int i = 0; i < data_blocks->count; ++i) {
        auto data_block = &data_blocks->data[i];
        if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
            process_hierarchical_data(data_block, log_struct_list, io_data);
        }
    }

    freeRequestBlock(request_block);

    return err;
}

int handle_request_fat(RequestBlock* request_block, RequestBlock* request_block0, ClientBlock* client_block,
                       ServerBlock* server_block, MetaDataBlock* metadata_block, DataBlockList* data_blocks,
                       Actions* actions_desc, Actions* actions_sig)
{
    UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #1 Loop\n");

    copy_request_block(request_block, *request_block0);

    int err = 0;

    print_client_block(*client_block);
    print_server_block(*server_block);
    print_request_block(*request_block);

    //----------------------------------------------------------------------
    // Initialise Data Structures

    init_data_source(&metadata_block->data_source);
    init_signal_desc(&metadata_block->signal_desc);
    init_signal(&metadata_block->signal_rec);

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data plug-in to use
    // Decide on Authentication procedure

    protocol_version = server_version;
    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[i];
        if (protocol_version >= 6) {
            if ((err = udaServerPlugin(request, &metadata_block->data_source, &metadata_block->signal_desc, &pluginList,
                                       getServerEnvironment())) != 0) {
                return err;
            }
        } else {
            if ((err = udaServerLegacyPlugin(request, &metadata_block->data_source, &metadata_block->signal_desc)) !=
                0) {
                return err;
            }
        }
    }

    //------------------------------------------------------------------------------------------------
    // Query the Database: Internal or External Data Sources
    // Read the Data or Create the Composite/Derived Data
    // Apply XML Actions to Data

    int depth = 0;

    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[i];
        assert(i == data_blocks->count);
        data_blocks->data = (DataBlock*)realloc(data_blocks->data, (data_blocks->count + 1) * sizeof(DataBlock));
        auto data_block = &data_blocks->data[i];
        init_data_block(data_block);
        err = get_data(&depth, request, *client_block, data_block, &metadata_block->data_source,
                         &metadata_block->signal_rec, &metadata_block->signal_desc, actions_desc, actions_sig,
                         &pluginList, log_malloc_list, user_defined_type_list, &socket_list, protocol_version);
        ++data_blocks->count;
    }

    if (err != 0) {
        return err;
    }

    DataSource* data_source = &metadata_block->data_source;
    SignalDesc* signal_desc = &metadata_block->signal_desc;
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
    print_request_block(*request_block);
    print_data_source(*data_source);
    print_signal(metadata_block->signal_rec);
    print_signal_desc(*signal_desc);
    print_data_block_list(*data_blocks);
    print_error_stack();
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");

    //------------------------------------------------------------------------------------------------
    // Server-Side Data Processing

    if (client_block->get_dimdble || client_block->get_timedble || client_block->get_scalar) {
        for (int i = 0; i < data_blocks->count; ++i) {
            auto data_block = &data_blocks->data[i];
            if (serverProcessing(*client_block, data_block) != 0) {
                UDA_THROW_ERROR(779, "Server-Side Processing Error");
            }
        }
    }

    //----------------------------------------------------------------------------
    // End of Error Trap #1
    // If an error has occued within this trap, then a problem occured accessing data
    // The server block should be returned with the error stack

    return err;
}

int do_fat_server_closedown(ServerBlock* server_block, DataBlockList* data_blocks, Actions* actions_desc,
                            Actions* actions_sig, DataBlockList* data_blocks0)
{
    //----------------------------------------------------------------------------
    // Free Plugin List and Close all open library entries

    // freePluginList(&pluginList);

    //----------------------------------------------------------------------------
    // Free Actions Heap

    free_actions(actions_desc);
    free_actions(actions_sig);

    //----------------------------------------------------------------------------

    concat_error(&server_block->idamerrorstack); // Update Server State with Global Error Stack
    close_error();

    *data_blocks0 = *data_blocks;

    print_data_block_list(*data_blocks0);

    return 0;
}

int startup_fat_server(ServerBlock* server_block, UserDefinedTypeList& parseduserdefinedtypelist)
{
    static int socket_list_initialised = 0;
    static int plugin_list_initialised = 0;
    // static int fileParsed = 0;

    //-------------------------------------------------------------------------
    // Open and Initialise the Socket List (Once Only)

    if (!socket_list_initialised) {
        init_socket_list(&socket_list);
        socket_list_initialised = 1;
    }

    //----------------------------------------------------------------------
    // Initialise General Structure Passing

    get_initial_user_defined_type_list(&user_defined_type_list);
    parseduserdefinedtypelist = *user_defined_type_list;
    user_defined_type_list = nullptr; // Startup State

    /*
    // this step needs doing once only - the first time a generalised user defined structure is encountered.
    // For FAT clients use a static state variable to prevent multiple parsing

    if (!fileParsed) {
        fileParsed = 1;

        init_user_defined_type_list(&parseduserdefinedtypelist);
        userdefinedtypelist = &parseduserdefinedtypelist; // Switch before Parsing input file

        char* token = nullptr;
        if ((token = getenv("UDA_SARRAY_CONFIG")) == nullptr) {
            UDA_THROW_ERROR(999, "No Environment variable UDA_SARRAY_CONFIG");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Parsing structure definition file: %s\n", token);
        parseIncludeFile(userdefinedtypelist, token); // file containing the SArray structure definition
        parseduserdefinedtypelist = *userdefinedtypelist; // Switch back
        print_user_defined_type_list(parseduserdefinedtypelist);
    }
    */

    //----------------------------------------------------------------------
    // Initialise the Data Reader Plugin list

    if (!plugin_list_initialised) {
        pluginList.count = 0;
        initPluginList(&pluginList, getServerEnvironment());
        plugin_list_initialised = 1;

        UDA_LOG(UDA_LOG_INFO, "List of Plugins available\n");
        for (int i = 0; i < pluginList.count; i++) {
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
