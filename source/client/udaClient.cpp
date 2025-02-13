#include "udaClient.h"

#include <algorithm>
#include <cstdlib>
#include <tuple>
#include <vector>

#include "cache/fileCache.h"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/printStructs.h"
#include "protocol/protocol.h"
#include "clientserver/uda_errors.h"
#include "clientserver/userid.h"
#include "logging/logging.h"
#include "structures/struct.h"
#include "uda/client.h"
#include "uda/structured.h"
#include "clientserver/version.h"

#include "connection.h"
#include "client_config.h"

#include <uda/version.h>

#include "closedown.h"

#ifdef FATCLIENT
#  include "clientserver/compressDim.h"
#  include "server/udaServer.h"
#  include "config/config.h"
#else
#  include "clientXDRStream.h"
#  include "protocol/xdrlib.h"

#  ifdef SSLAUTHENTICATION
#    include <authentication/udaClientSSL.h>

using namespace uda::authentication;
#  endif
#endif

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::logging;
using namespace uda::structures;

//------------------------------------------------ Static Globals ------------------------------------------------------

#if !defined(FATCLIENT) || !defined(NOLIBMEMCACHED)
static int protocol_version = 9;
#endif

constexpr int ClientVersion = UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);

//----------------------------------------------------------------------------------------------------------------------
// FATCLIENT objects shared with server code

#ifndef FATCLIENT
UserDefinedTypeList* g_user_defined_type_list = nullptr; // List of all known User Defined Structure Types
LogMallocList* g_log_malloc_list = nullptr;              // List of all Heap Allocations for Data
unsigned int g_last_malloc_index = 0;                    // Malloc Log search index last value
unsigned int* g_last_malloc_index_value = &g_last_malloc_index;
; // Preserve Malloc Log search index last value in GENERAL_STRUCT
XDR** g_client_input = nullptr;
XDR** g_client_output = nullptr;
LogStructList* g_log_struct_list = nullptr;
#endif // FATCLIENT

//----------------------------------------------------------------------------------------------------------------------

ClientBlock client_block;
ServerBlock server_block;

time_t tv_server_start = 0;
time_t tv_server_end = 0;

char client_username[StringLength] = "client";

int authentication_needed = 1; // Enable the mutual authentication conversation at startup

#ifndef FATCLIENT

void uda::client::setUserDefinedTypeList(UserDefinedTypeList* userdefinedtypelist_in)
{
    g_user_defined_type_list = userdefinedtypelist_in;
}

void uda::client::setLogMallocList(LogMallocList* logmalloclist_in)
{
    g_log_malloc_list = logmalloclist_in;
}

#else
void uda::client::setUserDefinedTypeList(UserDefinedTypeList* userdefinedtypelist_in) {}
void uda::client::setLogMallocList(LogMallocList* logmalloclist_in) {}

extern SOCKETLIST socket_list;
#endif

static std::vector<DataBlock> data_blocks = {};

int getCurrentDataBlockIndex()
{
    auto client_flags = udaClientFlags();
    if ((client_flags->flags & client_flags::ReuseLastHandle || client_flags->flags & client_flags::FreeReuseLastHandle) &&
        udaGetThreadLastHandle() >= 0) {
        return udaGetThreadLastHandle();
    }
    return data_blocks.size() - 1;
}

void free_data_blocks()
{
    data_blocks.clear();
    udaPutThreadLastHandle(-1);
}

int growDataBlocks()
{
    auto client_flags = udaClientFlags();
    if ((client_flags->flags & client_flags::ReuseLastHandle || client_flags->flags & client_flags::FreeReuseLastHandle) &&
        udaGetThreadLastHandle() >= 0) {
        return 0;
    }

    data_blocks.push_back({});
    init_data_block(&data_blocks.back());
    data_blocks.back().handle = data_blocks.size() - 1;

    udaPutThreadLastHandle(data_blocks.size() - 1);

    return 0;
}

static int findNewHandleIndex()
{
    for (int i = 0; i < (int)data_blocks.size(); i++) {
        if (data_blocks[i].handle == -1) {
            return i;
        }
    }
    return -1;
}

int getNewDataHandle()
{
    auto client_flags = udaClientFlags();
    int newHandleIndex = -1;

    if ((client_flags->flags & client_flags::ReuseLastHandle || client_flags->flags & client_flags::FreeReuseLastHandle) &&
        (newHandleIndex = udaGetThreadLastHandle()) >= 0) {
        if (client_flags->flags & client_flags::FreeReuseLastHandle) {
            udaFree(newHandleIndex);
        } else {
            // Application has responsibility for freeing heap in the Data Block
            init_data_block(&data_blocks[newHandleIndex]);
        }
        data_blocks[newHandleIndex].handle = newHandleIndex;
        return newHandleIndex;
    }

    if ((newHandleIndex = findNewHandleIndex()) < 0) { // Search for an unused handle or issue a new one
        newHandleIndex = data_blocks.size();
        data_blocks.push_back({});
        init_data_block(&data_blocks[newHandleIndex]);
        data_blocks[newHandleIndex].handle = newHandleIndex;
    } else {
        init_data_block(&data_blocks[newHandleIndex]);
        data_blocks[newHandleIndex].handle = newHandleIndex;
    }

    udaPutThreadLastHandle(newHandleIndex);
    return newHandleIndex;
}

DataBlock* uda::client::getDataBlock(int handle)
{
    if (handle < 0 || (unsigned int)handle >= data_blocks.size()) {
        return nullptr;
    }
    return &data_blocks[handle];
}

int newDataHandle()
{
    int newHandleIndex = -1;
    auto client_flags = udaClientFlags();

    if ((client_flags->flags & client_flags::ReuseLastHandle || client_flags->flags & client_flags::FreeReuseLastHandle) &&
        (newHandleIndex = udaGetThreadLastHandle()) >= 0) {
        if (client_flags->flags & client_flags::FreeReuseLastHandle) {
            udaFree(newHandleIndex);
        } else {
            // Application has responsibility for freeing heap in the Data Block
            init_data_block(&data_blocks[newHandleIndex]);
        }
        data_blocks[newHandleIndex].handle = newHandleIndex;
        return newHandleIndex;
    }

    if ((newHandleIndex = findNewHandleIndex()) < 0) { // Search for an unused handle or issue a new one
        newHandleIndex = data_blocks.size();
        data_blocks.push_back({});
        init_data_block(&data_blocks[newHandleIndex]);
        data_blocks[newHandleIndex].handle = newHandleIndex;
    } else {
        init_data_block(&data_blocks[newHandleIndex]);
        data_blocks[newHandleIndex].handle = newHandleIndex;
    }

    udaPutThreadLastHandle(newHandleIndex);
    return newHandleIndex;
}

void uda::client::updateClientBlock(ClientBlock* str, const CLIENT_FLAGS* client_flags, unsigned int private_flags)
{
    // other structure elements are set when the structure is initialised

    // ****** LEGACY ******

    str->timeout = client_flags->user_timeout;
    str->clientFlags = client_flags->flags;
    str->altRank = client_flags->alt_rank;
    str->get_datadble = client_flags->get_datadble;
    str->get_dimdble = client_flags->get_dimdble;
    str->get_timedble = client_flags->get_timedble;
    str->get_scalar = client_flags->get_scalar;
    str->get_bytes = client_flags->get_bytes;
    str->get_bad = client_flags->get_bad;
    str->get_meta = client_flags->get_meta;
    str->get_asis = client_flags->get_asis;
    str->get_uncal = client_flags->get_uncal;
    str->get_notoff = client_flags->get_notoff;
    str->get_nodimdata = client_flags->get_nodimdata;
    str->privateFlags = private_flags;
}

#ifndef NOLIBMEMCACHED
/**
 * Check the local cache for the data (GET methods only - Note: some GET methods may disguise PUT methods!)
 *
 * @param request_data
 * @param p_data_block
 * @return
 */
int check_file_cache(const RequestData* request_data, DataBlock** p_data_block, LogMallocList* log_malloc_list,
                     UserDefinedTypeList* user_defined_type_list, LogStructList* log_struct_list,
                     CLIENT_FLAGS* client_flags, unsigned int private_flags, int malloc_source)
{
    if (client_flags->flags & client_flags::FileCache && !request_data->put) {
        // Query the cache for the Data
        DataBlock* data = udaFileCacheRead(request_data, log_malloc_list, user_defined_type_list, protocol_version,
                                           log_struct_list, private_flags, malloc_source);

        if (data != nullptr) {
            // Success
            int data_block_idx = udaGetNewDataHandle();

            if (data_block_idx < 0) { // Error
                return -data_block_idx;
            }

            *p_data_block = udaGetDataBlock(data_block_idx);

            memcpy(*p_data_block, data, sizeof(DataBlock));
            free(data);

            return data_block_idx;
        }
    }

    return -1;
}

int check_mem_cache(uda::cache::UdaCache* cache, RequestData* request_data, DataBlock** p_data_block,
                    LogMallocList* log_malloc_list, UserDefinedTypeList* user_defined_type_list,
                    LogStructList* log_struct_list, CLIENT_FLAGS* client_flags, unsigned int private_flags,
                    int malloc_source)
{
    // Check Client Properties for permission to cache
    if ((client_flags->flags & client_flags::Cache) && !request_data->put) {

        // Open the Cache
        if (cache == nullptr) {
            cache = uda::cache::open_cache();
        }

        // Query the cache for the Data
        DataBlock* data =
            cache_read(cache, request_data, log_malloc_list, user_defined_type_list, *getIdamClientEnvironment(),
                       protocol_version, client_flags->flags, log_struct_list, private_flags, malloc_source);

        if (data != nullptr) {
            // Success
            int data_block_idx = udaGetNewDataHandle();

            if (data_block_idx < 0) { // Error
                return -data_block_idx;
            }

            *p_data_block = udaGetDataBlock(data_block_idx);

            memcpy(*p_data_block, data, sizeof(DataBlock));
            free(data);

            return data_block_idx;
        }
    }

    return -1;
}
#endif

void copyDataBlock(DataBlock* str, DataBlock* in)
{
    *str = *in;
    memcpy(str->errparams, in->errparams, MaxErrParams);
    memcpy(str->data_units, in->data_units, StringLength);
    memcpy(str->data_label, in->data_label, StringLength);
    memcpy(str->data_desc, in->data_desc, StringLength);
    memcpy(str->error_msg, in->error_msg, StringLength);
    init_client_block(&str->client_block, 0, "");
}

void copyClientBlock(ClientBlock* str, const CLIENT_FLAGS* client_flags)
{
    // other structure elements are set when the structure is initialised

    // ****** LEGACY ******

    str->timeout = client_flags->user_timeout;
    str->clientFlags = client_flags->flags;
    str->altRank = client_flags->alt_rank;
    str->get_datadble = client_flags->get_datadble;
    str->get_dimdble = client_flags->get_dimdble;
    str->get_timedble = client_flags->get_timedble;
    str->get_scalar = client_flags->get_scalar;
    str->get_bytes = client_flags->get_bytes;
    str->get_bad = client_flags->get_bad;
    str->get_meta = client_flags->get_meta;
    str->get_asis = client_flags->get_asis;
    str->get_uncal = client_flags->get_uncal;
    str->get_notoff = client_flags->get_notoff;
    str->get_nodimdata = client_flags->get_nodimdata;
}

/*
 *** stop using Data_Block_Count-1 to identify the new DataBlock structure, use newHandleIndex
 *** retain Data_Block_Count as this identifies the upper value of valid handles issued
 *** search for a handle < 0
 *** when freed/initialised, the DataBlock handle should be set to -1
 *** are there any instances where the data_block handle value is used?
 */

#ifndef FATCLIENT
/*
 * Fetch Hierarchical Data Structures
 *
 * manages it's own client/server conversation current buffer status in protocolXML2 ...
 */
static int fetchHierarchicalData(XDR* client_input, DataBlock* data_block, LogStructList* log_struct_list,
                                 unsigned int private_flags, int malloc_source)
{
    if (data_block->data_type == UDA_TYPE_COMPOUND && data_block->opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        ProtocolId protocol_id = ProtocolId::Start;
        if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = ProtocolId::Meta;
        } else if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDROBJECT) {
            protocol_id = ProtocolId::Structures;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Receiving Hierarchical Data Structure from Server");

        int err = 0;
        if ((err =
                 protocol2(client_input, protocol_id, XDRStreamDirection::Receive, nullptr, g_log_malloc_list, g_user_defined_type_list,
                           data_block, protocol_version, log_struct_list, private_flags, malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Client Side Protocol Error (Opaque Structure Type)");
            return err;
        }
    }

    return 0;
}
#endif

static int allocMeta(DataSystem** data_system, SystemConfig** system_config, DataSource** data_source,
                     Signal** signal_rec, SignalDesc** signal_desc)
{
    int err = 0;

    // Allocate memory for the Meta Data
    *data_system = (DataSystem*)malloc(sizeof(DataSystem));
    *system_config = (SystemConfig*)malloc(sizeof(SystemConfig));
    *data_source = (DataSource*)malloc(sizeof(DataSource));
    *signal_rec = (Signal*)malloc(sizeof(Signal));
    *signal_desc = (SignalDesc*)malloc(sizeof(SignalDesc));

    if (*data_system == nullptr || *system_config == nullptr || *data_source == nullptr || *signal_rec == nullptr ||
        *signal_desc == nullptr) {
        err = (int)ServerSideError::ErrorAllocatingMetaDataHeap;
        add_error(ErrorType::Code, __func__, err, "Error Allocating Heap for Meta Data");
        return err;
    }

    return err;
}

static int fetchMeta(XDR* client_input, DataSystem* data_system, SystemConfig* system_config, DataSource* data_source,
                     Signal* signal_rec, SignalDesc* signal_desc, LogStructList* log_struct_list,
                     unsigned int private_flags, int malloc_source)
{
    int err = 0;

#ifndef FATCLIENT // <========================== Client Server Code Only
    if ((err = protocol2(client_input, ProtocolId::DataSystem, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                         g_user_defined_type_list, data_system, protocol_version, log_struct_list, private_flags,
                         malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 4 Error (Data System)");
        return err;
    }
    print_data_system(*data_system);

    if ((err = protocol2(client_input, ProtocolId::SystemConfig, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                         g_user_defined_type_list, system_config, protocol_version, log_struct_list, private_flags,
                         malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 5 Error (System Config)");
        return err;
    }
    print_system_config(*system_config);

    if ((err = protocol2(client_input, ProtocolId::DataSource, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                         g_user_defined_type_list, data_source, protocol_version, log_struct_list, private_flags,
                         malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 6 Error (Data Source)");
        return err;
    }
    print_data_source(*data_source);

    if ((err = protocol2(client_input, ProtocolId::Signal, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                         g_user_defined_type_list, signal_rec, protocol_version, log_struct_list, private_flags,
                         malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 7 Error (Signal)");
        return err;
    }
    print_signal(*signal_rec);

    if ((err = protocol2(client_input, ProtocolId::SignalDesc, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                         g_user_defined_type_list, signal_desc, protocol_version, log_struct_list, private_flags,
                         malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 8 Error (Signal Desc)");
        return err;
    }
    print_signal_desc(*signal_desc);
#endif

    return err;
}

CLIENT_FLAGS* uda::client::udaClientFlags()
{
    static CLIENT_FLAGS client_flags = {};
    return &client_flags;
}

unsigned int* uda::client::udaPrivateFlags()
{
    static unsigned int private_flags = 0;
    return &private_flags;
}

int uda::client::udaClient(RequestBlock* request_block, int* indices)
{
    // Efficient reduced (filled) tcp packet protocol for efficiency over large RTT fat pipes
    // This client version will only be able to communicate with a version 7+ server

    int err;

    int serverside = 0;

#ifndef FATCLIENT
    bool allocMetaHeap = false;
#  ifndef SECURITYENABLED
    static int startupStates;
#  endif // !SECURITYENABLED
#endif   // !FATCLIENT

    static bool system_startup = true;

    static bool reopen_logs = false;
    static XDR* client_input = nullptr;
    static XDR* client_output = nullptr;
    static int malloc_source = UDA_MALLOC_SOURCE_NONE;

#ifndef FATCLIENT
    g_client_input = &client_input;   // needed for udaFreeAll
    g_client_output = &client_output; // needed for udaFreeAll
#endif

    LogStructList log_struct_list;
    init_log_struct_list(&log_struct_list);

    unsigned int* private_flags = udaPrivateFlags();
    CLIENT_FLAGS* client_flags = udaClientFlags();
    client_flags->alt_rank = 0;
    client_flags->user_timeout = TimeOut;

    time_t protocol_time; // Time a Conversation Occured

    if (system_startup && getenv("UDA_TIMEOUT")) {
        client_flags->user_timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }

    //------------------------------------------------------------------------------
    // Open the Socket if this is the First call for Data or the server is known to be dead
    //
    // If this is Not the First Call then assume the server is asleep and waiting for a request
    // Check the time since last Call for Data does not exceed a threshold:
    //  If threshold exceeded then assume the Server has closed down.
    //      => Close old Socket (the server will automatically die if awake)
    //      => Open New Connection and use the Startup Protocol
    //
    //------------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    // Initialise the Error Stack before Accessing Data

    if (tv_server_start != 0) {
        free_error_stack(&server_block.idamerrorstack); // Free Previous Stack Heap
    }

    init_server_block(&server_block, 0); // Reset previous Error Messages from the Server & Free Heap
    init_error_stack();

    //-------------------------------------------------------------------------
    // Initialise Protocol Timings (in Debug Mode only)

    time(&protocol_time);

    DataBlockList cached_data_block_list;
    cached_data_block_list.count = request_block->num_requests;
    cached_data_block_list.data = (DataBlock*)malloc(cached_data_block_list.count * sizeof(DataBlock));

    //------------------------------------------------------------------------------
    // Error Trap: Some Errors are Fatal => Server Destroyed & Connections Closed

    // *** change error management when large RTT involved - why destroy the server if not warranted?

    bool data_received = false;
    std::vector<int> data_block_indices(request_block->num_requests);
    std::fill(data_block_indices.begin(), data_block_indices.end(), -1);

    DataSystem* data_system = nullptr;
    SystemConfig* system_config = nullptr;
    DataSource* data_source = nullptr;
    Signal* signal_rec = nullptr;
    SignalDesc* signal_desc = nullptr;

    do {

        if (tv_server_start == 0) {
            time(&tv_server_start); // First Call: Start the Clock
        }

#ifndef FATCLIENT // <========================== Client Server Code Only
#  ifndef NOLIBMEMCACHED
        static uda::cache::UdaCache* cache;

        int num_cached = 0;
        for (int i = 0; i < request_block->num_requests; ++i) {
            auto request = &request_block->requests[i];
            DataBlock* data_block = &cached_data_block_list.data[i];
            int rc = check_file_cache(request, &data_block, g_log_malloc_list, g_user_defined_type_list,
                                      &log_struct_list, client_flags, *private_flags, malloc_source);
            if (rc >= 0) {
                request_block->requests[i].request = Request::Cached;
                ++num_cached;
                continue;
            }
            rc = check_mem_cache(cache, request, &data_block, g_log_malloc_list, g_user_defined_type_list,
                                 &log_struct_list, client_flags, *private_flags, malloc_source);
            if (rc >= 0) {
                request_block->requests[i].request = Request::Cached;
                ++num_cached;
                continue;
            }
        }
#  endif // !NOLIBMEMCACHED

        //-------------------------------------------------------------------------
        // Manage Multiple UDA Server connections ...
        //
        // Instance a new server on the same Host/Port or on a different Host/port

        auto config = client_config();
        auto reconnect = config->get("connection.reconnect").as_or_default(false);
        auto change_socket = config->get("connection.change_socket").as_or_default(false);

        if (reconnect || change_socket) {
            err = ::reconnect(*config, &client_input, &client_output, &tv_server_start, &client_flags->user_timeout);
            if (err) {
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Server Age (secs) = Time since the Server was last called

        time(&tv_server_end);
        long age = (long)tv_server_end - (long)tv_server_start;

        UDA_LOG(UDA_LOG_DEBUG, "Start: {}    End: {}", (long)tv_server_start, (long)tv_server_end);
        UDA_LOG(UDA_LOG_DEBUG, "Server Age: {}", age);

        //-------------------------------------------------------------------------
        // Server State: Is the Server Dead? (Age Dependent)

        bool init_server = true;

        if (age >= client_flags->user_timeout - 2) {
            // Assume the Server has Self-Destructed so Instantiate a New Server
            UDA_LOG(UDA_LOG_DEBUG, "Server Age Limit Reached {}", (long)age);
            UDA_LOG(UDA_LOG_DEBUG, "Server Closed and New Instance Started");

            // Close the Existing Socket and XDR Stream: Reopening will Instance a New Server
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input, client_output, &reopen_logs);
        } else if (connectionOpen()) {
            // Assume the Server is Still Alive
            if (client_output->x_ops == nullptr || client_input->x_ops == nullptr) {
                add_error(ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
                UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
                closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input, client_output, &reopen_logs);
                init_server = true;
            } else {
                init_server = false;
                xdrrec_eof(client_input); // Flush input socket
            }
        }

        //-------------------------------------------------------------------------
        // Open a Socket and Connect to the UDA Data Server (Multiple Servers?)

        if (init_server) {
            authentication_needed = 1;
#  if !defined(FATCLIENT) && !defined(SECURITYENABLED)
            startupStates = 0;
#  endif
            time(&tv_server_start); // Start the Clock again: Age of Server

            //-------------------------------------------------------------------------
            // Connect to the server with SSL (X509) authentication

#  if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            // Create the SSL binding and context, and verify the server certificate
            if ((err = initUdaClientSSL()) != 0) {
                break;
            }
#  endif

            //-------------------------------------------------------------------------
            // Create the XDR Record Streams
            std::tie(client_input, client_output) = clientCreateXDRStream();

            if ((createConnection(client_input, client_output, &tv_server_start, client_flags->user_timeout)) != 0) {
                err = NO_SOCKET_CONNECTION;
                add_error(ErrorType::Code, __func__, err, "No Socket Connection to Server");
                break;
            }
        }

        //-------------------------------------------------------------------------
        // Connect to the server with SSL (X509) authentication

#  if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
        // Create the SSL binding and context, and verify the server certificate
        if ((err = startUdaClientSSL()) != 0) {
            break;
        }
#  endif

        //-------------------------------------------------------------------------

#else  // <========================== End of Client Server Code Only
        bool init_server = true;
#endif // <========================== End of FatClient Code Only

        //-------------------------------------------------------------------------
        // Initialise the Client/Server Structures

        if (init_server && system_startup) {
            user_id(client_username);
            init_client_block(&client_block, ClientVersion, client_username);
            system_startup = false; // Don't call again!
        }

        // Update the Private Flags

        char* env = nullptr;

#ifndef FATCLIENT
        if ((env = getenv("UDA_PRIVATEFLAGS")) != nullptr) {
            udaSetPrivateFlag(atoi(env));
        }
#endif

        updateClientBlock(&client_block, client_flags, *private_flags); // Allows User to Change Properties at run-time

        // Operating System Name

        if ((env = getenv("OSTYPE")) != nullptr) {
            strcpy(client_block.OSName, env);
        }

        // Client's study DOI

        if ((env = getenv("UDA_CLIENT_DOI")) != nullptr) {
            strcpy(client_block.DOI, env);
        }

        print_client_block(client_block);

        //------------------------------------------------------------------------------
        // User Authentication at startup

#ifndef FATCLIENT // <========================== Client Server Code Only

#  ifdef SECURITYENABLED

        // Client/Server connection is established but neither the server nor the user identity has been authenticated.
        // Client initiates the process by sending an encrypted token with its claim of identity.
        // The multi-step dialogue between client and server tests the identity claims of both.
        // Authentication occurs at server startup only.
        // Once passed, the original data request is processed.

        // TBD:
        // Additional tests of identity will be included in consequent client/server dialogue to maintain authenticated
        // state. If the connection breaks and the client has to start a new connection to the same host, the last good
        // exchanged token (within a very short time window) can be used to byepass the multi-RTT cost of
        // authentication. Use asynchronous encrypted caching to manage these short-lived session tokens. Use a key
        // based on originating IP address and claimed identity.

        // When a server is a regular server and connects to another server as a data source, then the originating
        // server has to authenticate as if a client. The originating client has also to authenticate with the final
        // server.

        // When a server is acting as a simple proxy it passes all requests onwards without authentication - it does not
        // interpret the data access request.

        initSecurityBlock(&client_block.securityBlock);
        initSecurityBlock(&server_block.securityBlock);

        if (authenticationNeeded) {

            // generate token A and encrypt with the server public key
            // send Client_block: the client certificate and encrypted token A

            unsigned short authenticationStep = 1; // Client Certificate authenticated by server

            if ((err = clientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Client or Server Authentication Failed #1");
                break;
            }

            // receive server_block
            // decrypt tokens A, B using private key
            // Test token A is identical to that sent in step 1 => test server has a valid private key

            authenticationStep = 5;

            if ((err = clientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Client or Server Authentication Failed #5");
                break;
            }

            // encrypt token B with the server public key       => send proof client has a valid private key (paired
            // with public key from certificate) send client block

            authenticationStep = 6;

            if ((err = clientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Client or Server Authentication Failed #6");
                break;
            }

            // receive server_block
            // decrypt new token B using private key        => proof server has a valid private key == server
            // authenticated encrypt new token B with the server public key   => maintain authentication

            authenticationStep = 8;

            if ((err = clientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Client or Server Authentication Failed #8");
                break;
            }

            authenticationNeeded = 0; // Both Client and Server have been mutually authenticated
        }

#  else

        //-------------------------------------------------------------------------
        // Client and Server States at Startup only (1 RTT)
        // Will be passed during mutual authentication step

        if (!startupStates) {

            // Flush (mark as at EOF) the input socket buffer (before the exchange begins)

            ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block (proxy for authenticationStep = 6)

            if ((err = protocol2(client_output, protocol_id, XDRStreamDirection::Send, nullptr, g_log_malloc_list,
                                 g_user_defined_type_list, &client_block, protocol_version, &log_struct_list,
                                 *private_flags, malloc_source)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
                UDA_LOG(UDA_LOG_DEBUG, "Error Sending Client Block");
                break;
            }

            if (!(xdrrec_endofrecord(client_output, 1))) { // Send data now
                err = (int)ProtocolError::Error7;
                add_error(ErrorType::Code, __func__, err, "Protocol 7 Error (Client Block)");
                UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_endofrecord after Client Block");
                break;
            }

            // Flush (mark as at EOF) the input socket buffer (start of wait for data)

            // Wait for data, then position buffer reader to the start of a new record
            if (!(xdrrec_skiprecord(client_input))) {
                err = (int)ProtocolError::Error5;
                add_error(ErrorType::Code, __func__, err, "Protocol 5 Error (Server Block)");
                UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_skiprecord prior to Server Block");
                break;
            }

            protocol_id = ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement (proxy for
                                                     // authenticationStep = 8)

            if ((err = protocol2(client_input, protocol_id, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                                 g_user_defined_type_list, &server_block, protocol_version, &log_struct_list,
                                 *private_flags, malloc_source)) != 0) {
                add_error(ErrorType::Code, __func__, err, "Protocol 11 Error (Server Block #1)");
                // Assuming the server_block is corrupted, replace with a clean copy to avoid concatonation problems
                server_block.idamerrorstack.nerrors = 0;
                UDA_LOG(UDA_LOG_DEBUG, "Error receiving Server Block");
                break;
            }

            // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version
            // dependent)

            int rc = xdrrec_eof(client_input);

            UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
            UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} [1 => no more input]", rc);
            print_server_block(server_block);

            // Protocol Version: Lower of the client and server version numbers
            // This defines the set of elements within data structures passed between client and server
            // Must be the same on both sides of the socket
            protocol_version = std::min(get_protocol_version(client_block.version), get_protocol_version(server_block.version));

            if (server_block.idamerrorstack.nerrors > 0) {
                err = server_block.error_stack[0].code; // Problem on the Server Side!
                break;
            }

            startupStates = 1;

        } // startupStates

#  endif // not SECURITYENABLED

        //-------------------------------------------------------------------------
        // Check the Server version is not older than this client's version

        UDA_LOG(UDA_LOG_DEBUG, "protocolVersion {}", protocol_version);
        UDA_LOG(UDA_LOG_DEBUG, "Client Version  {}", client_block.version);
        UDA_LOG(UDA_LOG_DEBUG, "Server Version  {}", server_block.version);

        //-------------------------------------------------------------------------
        // Flush to EOF the input buffer (start of wait for new data) necessary when Zero data waiting but not an EOF!

        int rc = 0;
        if (!(rc = xdrrec_eof(client_input))) { // Test for an EOF

            UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} => more input when none expected!", rc);

            int count = 0;
            char temp;

            do {
                rc = xdr_char(client_input, &temp); // Flush the input (limit to 64 bytes)

                if (rc) {
                    UDA_LOG(UDA_LOG_DEBUG, "[{}] [{}]", count++, temp);
                }
            } while (rc && count < 64);

            if (count > 0) { // Error if data is waiting
                err = 999;
                add_error(ErrorType::Code, __func__, err,
                          "Data waiting in the input data buffer when none expected! Please contact the system "
                          "administrator.");
                UDA_LOG(UDA_LOG_DEBUG, "[{}] excess data bytes waiting in input buffer!", count++);
                break;
            }

            rc = xdrrec_eof(client_input); // Test for an EOF

            if (!rc) {
                rc = xdrrec_skiprecord(client_input); // Flush the input buffer (Zero data waiting but not an EOF!)
            }

            rc = xdrrec_eof(client_input); // Test for an EOF

            if (!rc) {
                err = 999;
                add_error(ErrorType::Code, __func__, err,
                          "Corrupted input data stream! Please contact the system administrator.");
                UDA_LOG(UDA_LOG_DEBUG, "Unable to flush input buffer!!!");
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = 1 => no more input, buffer flushed.");
        }

        //-------------------------------------------------------------------------
        // Send the Client Block

        ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block

        if ((err = protocol2(client_output, protocol_id, XDRStreamDirection::Send, nullptr, g_log_malloc_list, g_user_defined_type_list,
                             &client_block, protocol_version, &log_struct_list, *private_flags, malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
            break;
        }

        //-------------------------------------------------------------------------
        // Send the Client Request

        protocol_id = ProtocolId::RequestBlock; // This is what the Client Wants

        if ((err = protocol2(client_output, protocol_id, XDRStreamDirection::Send, nullptr, g_log_malloc_list, g_user_defined_type_list,
                             request_block, protocol_version, &log_struct_list, *private_flags, malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Protocol 1 Error (Request Block)");
            break;
        }

        //------------------------------------------------------------------------------
        // Pass the PUTDATA structure

        for (int i = 0; i < request_block->num_requests; ++i) {
            RequestData* request = &request_block->requests[i];

            if (request->put) {
                protocol_id = ProtocolId::PutdataBlockList;

                if ((err = protocol2(client_output, protocol_id, XDRStreamDirection::Send, nullptr, g_log_malloc_list,
                                     g_user_defined_type_list, &(request->putDataBlockList), protocol_version,
                                     &log_struct_list, *private_flags, malloc_source)) != 0) {
                    add_error(ErrorType::Code, __func__, err,
                              "Protocol 1 Error (sending putDataBlockList from Request Block)");
                    break;
                }
            }
        }

        //------------------------------------------------------------------------------
        // Send the Full TCP packet and wait for the returned data

        if (!(rc = xdrrec_endofrecord(client_output, 1))) {
            err = (int)ProtocolError::Error7;
            add_error(ErrorType::Code, __func__, err, "Protocol 7 Error (Request Block & putDataBlockList)");
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "****** Outgoing tcp packet sent without error. Waiting for data.");

        if (!xdrrec_skiprecord(client_input)) {
            err = (int)ProtocolError::Error5;
            add_error(ErrorType::Code, __func__, err, " Protocol 5 Error (Server & Data Structures)");
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Reading...");

        //------------------------------------------------------------------------------
        // Receive the Server State/Aknowledgement that the Data has been Accessed
        // Just in case the Server has crashed!

        UDA_LOG(UDA_LOG_DEBUG, "Waiting for Server Status Block");

        if ((err = protocol2(client_input, ProtocolId::ServerBlock, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                             g_user_defined_type_list, &server_block, protocol_version, &log_struct_list,
                             *private_flags, malloc_source)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Protocol 11 Error (Server Block #2) = {}", err);
            add_error(ErrorType::Code, __func__, err, " Protocol 11 Error (Server Block #2)");
            // Assuming the server_block is corrupted, replace with a clean copy to avoid future concatonation problems
            server_block.idamerrorstack.nerrors = 0;
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
        print_server_block(server_block);

        serverside = 0;

        if (server_block.idamerrorstack.nerrors > 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
            err = server_block.error_stack[0].code; // Problem on the Server Side!
            UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
            serverside = 1; // Most Server Side errors are benign so don't close the server
            break;
        }

#endif // not FATCLIENT            // <===== End of Client Server Only Code

        //------------------------------------------------------------------------------
        // Return Database Meta Data if User Requests it

#ifndef FATCLIENT // <========================== Client Server Code Only
        allocMetaHeap = false;
#endif // <===== End of Client Server Only Code

        if (client_block.get_meta) {
            if ((err = allocMeta(&data_system, &system_config, &data_source, &signal_rec, &signal_desc)) != 0) {
                break;
            }
            if ((err = fetchMeta(client_input, data_system, system_config, data_source, signal_rec, signal_desc,
                                 &log_struct_list, *private_flags, malloc_source)) != 0) {
                break;
            }
#ifndef FATCLIENT // <========================== Client Server Code Only
            allocMetaHeap = true;
#endif // <===== End of Client Server Only Code
        }

#ifndef FATCLIENT // <========================== Client Server Code Only

        //------------------------------------------------------------------------------
        // Fetch the data Block

        DataBlockList recv_data_block_list;

        if ((err = protocol2(client_input, ProtocolId::DataBlockList, XDRStreamDirection::Receive, nullptr, g_log_malloc_list,
                             g_user_defined_type_list, &recv_data_block_list, protocol_version, &log_struct_list,
                             *private_flags, malloc_source)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");

            add_error(ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
            break;
        }

        // Flush (mark as at EOF) the input socket buffer (All regular Data has been received)

        print_data_block_list(recv_data_block_list);

        for (int i = 0; i < request_block->num_requests; ++i) {
            //------------------------------------------------------------------------------
            // Allocate memory for the Data Block Structure
            // Re-use existing stale Data Blocks
            int data_block_idx = getNewDataHandle();

            if (data_block_idx < 0) { // Error
                data_block_indices[i] = -data_block_idx;
                continue;
            }

            DataBlock* data_block = getDataBlock(data_block_idx);

            //           DataBlock* in_data;
            //           if (request_block->requests[i].request == Request::Cached) {
            //               in_data = &cached_data_block_list.data[i];
            //           } else {
            //               in_data = &recv_data_block_list.data[recv_idx];
            //               ++recv_idx;
            //           }

            copyDataBlock(data_block, &recv_data_block_list.data[i]);
            copyClientBlock(&data_block->client_block, client_flags);

            if (client_block.get_meta) {
                if ((err = allocMeta(&data_block->data_system, &data_block->system_config, &data_block->data_source,
                                     &data_block->signal_rec, &data_block->signal_desc)) != 0) {
                    break;
                }
                *data_block->data_system = *data_system;
                *data_block->system_config = *system_config;
                *data_block->data_source = *data_source;
                *data_block->signal_rec = *signal_rec;
                *data_block->signal_desc = *signal_desc;
            }

            fetchHierarchicalData(client_input, data_block, &log_struct_list, *private_flags, malloc_source);

            //------------------------------------------------------------------------------
            // Cache the data if the server has passed permission and the application (client) has enabled caching
            if (client_flags->flags & client_flags::FileCache) {
                udaFileCacheWrite(data_block, request_block, g_log_malloc_list, g_user_defined_type_list,
                                  protocol_version, &log_struct_list, *private_flags, malloc_source);
            }

#  ifndef NOLIBMEMCACHED
#    ifdef CACHEDEV
            if (cache != nullptr && clientFlags & client_flags::Cache &&
                data_block.cachePermission == PluginCachePermission::OkToCache) {
#    else
            if (cache != nullptr && client_flags->flags & client_flags::Cache) {
#    endif
                cache_write(cache, &request_block->requests[i], data_block, g_log_malloc_list, g_user_defined_type_list,
                            *environment, protocol_version, client_flags->flags, &log_struct_list, *private_flags,
                            malloc_source);
            }
#  endif // !NOLIBMEMCACHED

            data_block_indices[i] = data_block_idx;
        }

        data_received = true;
        free(recv_data_block_list.data);

        UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Structure Block Received");

#else // <========================== End of Client Server Code Only (not FATCLIENT)

        //------------------------------------------------------------------------------
        // Fat Client Server

        DataBlockList data_block_list0;
        init_data_block_list(&data_block_list0);
        uda::config::Config config = {};
        err = uda::server::fat_server(config, client_block, &server_block, request_block, &data_block_list0);

        for (int i = 0; i < data_block_list0.count; ++i) {
            DataBlock* data_block0 = &data_block_list0.data[i];

            int data_block_idx = getNewDataHandle();
            DataBlock* data_block = getDataBlock(data_block_idx); // data blocks may have been realloc'ed
            copyDataBlock(data_block, data_block0);

            if (err != 0 || server_block.idamerrorstack.nerrors > 0) {
                if (err == 0) {
                    err = SERVER_BLOCK_ERROR;
                }

                UDA_LOG(UDA_LOG_ERROR, "Error Returned from Data Server {}", err);
                break;
            }

            // Decompresss Dimensional Data

            for (int i = 0; i < (int)data_block->rank; i++) {                     // Expand Compressed Regular Vector
                err = uda::client_server::uncompress_dim(&(data_block->dims[i])); // Allocate Heap as required
                err = 0;                                                          // Need to Test for Error Condition!
            }

            print_data_block(*data_block);
        }

#endif // <========================== End of FatClient Code Only
       //------------------------------------------------------------------------------
       // End of Error Trap Loop

    } while (0);

    // 4 Possible Error States:
    //
    //  err == 0; newHandle = 0;   Cannot Happen: Close Server & Return an Error!
    //  err == 0; newHandle = 1;   Sleep Server & Return Handle
    //  err != 0; newHandle = 0;   Close Server (unless it has occurred server side) & Return -err
    //  err != 0; newHandle = 1;   Close Server (unless it has occurred server side) & Return Handle (Contains Error)

    UDA_LOG(UDA_LOG_DEBUG, "Error Code at end of Error Trap: {}", err);
    UDA_LOG(UDA_LOG_DEBUG, "serverside                     : {}", serverside);
    free(cached_data_block_list.data);

    cached_data_block_list.data = nullptr;

#ifndef FATCLIENT // <========================== Client Server Code Only

    //------------------------------------------------------------------------------
    // Close all File Handles, Streams, sockets and Free Heap Memory

    if (data_received) {
        if (err != 0 && !serverside) {
            // Close Socket & XDR Streams but Not Files
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input, client_output, &reopen_logs);
        }

        for (auto data_block_idx : data_block_indices) {
            if (err == 0 && (udaGetDataStatus(data_block_idx)) == MIN_STATUS && !client_flags->get_bad) {
                // If Data are not usable, flag the client
                add_error(ErrorType::Code, __func__, DATA_STATUS_BAD,
                          "Data Status is BAD ... Data are Not Usable!");

                DataBlock* data_block = getDataBlock(data_block_idx);
                if (data_block->errcode == 0) {
                    // Don't over-rule a server side error
                    data_block->errcode = DATA_STATUS_BAD;
                    strcpy(data_block->error_msg, "Data Status is BAD ... Data are Not Usable!");
                }
            }
        }

        //------------------------------------------------------------------------------
        // Concatenate Error Message Stacks & Write to the Error Log

        concat_error(&server_block.idamerrorstack);
        close_error();
        error_log(client_block, *request_block, &server_block.idamerrorstack);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = getDataBlock(data_block_idx);

            if (data_block->errcode == 0 && server_block.idamerrorstack.nerrors > 0) {
                data_block->errcode = udaGetServerErrorStackRecordCode(0);
                strcpy(data_block->error_msg, udaGetServerErrorStackRecordMsg(0));
            }
        }

        //------------------------------------------------------------------------------
        // Normal Exit: Return to Client

        std::copy(data_block_indices.begin(), data_block_indices.end(), indices);
        return 0;

        //------------------------------------------------------------------------------
        // Abnormal Exit: Return to Client

    } else {
        if (allocMetaHeap) {
            free(data_system);
            free(system_config);
            free(data_source);
            free(signal_rec);
            free(signal_desc);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Returning Error {}", err);

        if (err != 0 && !serverside) {
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input, client_output, &reopen_logs);
        }

        concat_error(&server_block.idamerrorstack);
        close_error();
        error_log(client_block, *request_block, &server_block.idamerrorstack);

        if (err == 0) {
            return ERROR_CONDITION_UNKNOWN;
        }

        return -abs(err); // Abnormal Exit
    }
}

#else // <========================== End of Client Server Code Only (not FATCLIENT)

    //------------------------------------------------------------------------------
    // If an error has occured: Close all File Handles, Streams, sockets and Free Heap Memory

    // rc = fflush(nullptr); // save anything ... the user might not follow correct procedure!

    if (data_received) {
        if (err != 0) {
            closedown(ClosedownType::CLOSE_SOCKETS, &socket_list, client_input, client_output, &reopen_logs);
        }

        for (auto data_block_idx : data_block_indices) {
            if (err == 0 && (udaGetDataStatus(data_block_idx) == MIN_STATUS) && !client_flags->get_bad) {
                // If Data are not usable, flag the client
                add_error(ErrorType::Code, __func__, DATA_STATUS_BAD,
                          "Data Status is BAD ... Data are Not Usable!");

                DataBlock* data_block = getDataBlock(data_block_idx);
                if (data_block->errcode == 0) {
                    // Don't over-rule a server side error
                    data_block->errcode = DATA_STATUS_BAD;
                    strcpy(data_block->error_msg, "Data Status is BAD ... Data are Not Usable!");
                }
            }
        }

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = getDataBlock(data_block_idx);

            if (err != 0 && data_block->errcode == 0) {
                add_error(ErrorType::Code, __func__, err, "Unknown Error");
                data_block->errcode = err;
            }
        }

        //------------------------------------------------------------------------------
        // Concatenate Error Message Stacks & Write to the Error Log

        concat_error(&server_block.idamerrorstack);
        close_error();

        error_log(client_block, *request_block, &server_block.idamerrorstack);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = getDataBlock(data_block_idx);

            if (data_block->errcode == 0 && server_block.idamerrorstack.nerrors > 0) {
                data_block->errcode = udaGetServerErrorStackRecordCode(0);
                strcpy(data_block->error_msg, udaGetServerErrorStackRecordMsg(0));
            }
        }

        //------------------------------------------------------------------------------
        // Normal Exit: Return to Client

        std::copy(data_block_indices.begin(), data_block_indices.end(), indices);
        return 0;

        //------------------------------------------------------------------------------
        // Abnormal Exit: Return to Client

    } else {

        UDA_LOG(UDA_LOG_DEBUG, "Returning Error {}", err);

        if (err != 0) {
            closedown(ClosedownType::CLOSE_SOCKETS, &socket_list, client_input, client_output, &reopen_logs);
        }

        concat_error(&server_block.idamerrorstack);
        error_log(client_block, *request_block, &server_block.idamerrorstack);

        if (err == 0) {
            return ERROR_CONDITION_UNKNOWN;
        }

        return -abs(err); // Abnormal Exit
    }
}

#endif // <========================== End of FatClient Code Only

void udaFree(int handle)
{

    // Free Heap Memory (Not the Data Blocks themselves: These will be re-used.)

    char* cptr;
    Dims* ddims;
    int rank;

    DataBlock* data_block = getDataBlock(handle);

    if (data_block == nullptr) {
        return;
    }

    // Free Hierarchical structured data first

    switch (data_block->opaque_type) {
        case UDA_OPAQUE_TYPE_XML_DOCUMENT: {
            if (data_block->opaque_block != nullptr) {
                free(data_block->opaque_block);
            }

            data_block->opaque_count = 0;
            data_block->opaque_block = nullptr;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            break;
        }

        case UDA_OPAQUE_TYPE_STRUCTURES: {
            if (data_block->opaque_block != nullptr) {
                auto general_block = (GeneralBlock*)data_block->opaque_block;

                if (general_block->userdefinedtypelist != nullptr) {
#ifndef FATCLIENT
                    if (g_user_defined_type_list ==
                        general_block->userdefinedtypelist) { // Is this the current setting?
                        udaFreeUserDefinedTypeList(g_user_defined_type_list);
                        free(g_user_defined_type_list);
                        g_user_defined_type_list = nullptr;
                    } else {
                        udaFreeUserDefinedTypeList(general_block->userdefinedtypelist);
                        free(general_block->userdefinedtypelist);
                    }
#else
                    udaFreeUserDefinedTypeList(general_block->userdefinedtypelist);
                    free(general_block->userdefinedtypelist);
#endif
                }

                if (general_block->logmalloclist != nullptr) {
#ifndef FATCLIENT
                    if (g_log_malloc_list == general_block->logmalloclist) {
                        udaFreeMallocLogList(g_log_malloc_list);
                        free(g_log_malloc_list);
                        g_log_malloc_list = nullptr;
                    } else {
                        udaFreeMallocLogList(general_block->logmalloclist);
                        free(general_block->logmalloclist);
                    }
#else
                    udaFreeMallocLogList(general_block->logmalloclist);
                    free(general_block->logmalloclist);
#endif
                }

#ifndef FATCLIENT
                if (general_block->userdefinedtype != nullptr) {
                    udaFreeUserDefinedType(general_block->userdefinedtype);
                    free(general_block->userdefinedtype);
                }

                free(general_block);
#endif
            }

            data_block->opaque_block = nullptr;
            data_block->data_type = UDA_TYPE_UNKNOWN;
            data_block->opaque_count = 0;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            data_block->data = nullptr;

            break;
        }

        case UDA_OPAQUE_TYPE_XDRFILE:
        case UDA_OPAQUE_TYPE_XDROBJECT: {
            if (data_block->opaque_block != nullptr) {
                free(data_block->opaque_block);
            }

            data_block->opaque_block = nullptr;
            data_block->data_type = UDA_TYPE_UNKNOWN;
            data_block->opaque_count = 0;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            data_block->data = nullptr;

            break;
        }

        default:
            break;
    }

    rank = data_block->rank;
    ddims = data_block->dims;

    if ((cptr = data_block->data) != nullptr) {
        free(cptr);
        data_block->data = nullptr; // Prevent another Free
    }

    if ((cptr = data_block->errhi) != nullptr) {
        free(cptr);
        data_block->errhi = nullptr;
    }

    if ((cptr = data_block->errlo) != nullptr) {
        free(cptr);
        data_block->errlo = nullptr;
    }

    if ((cptr = data_block->synthetic) != nullptr) {
        free(cptr);
        data_block->synthetic = nullptr;
    }

    if (data_block->data_system != nullptr) {
        free(data_block->data_system);
        data_block->data_system = nullptr;
    }

    if (data_block->system_config != nullptr) {
        free(data_block->system_config);
        data_block->system_config = nullptr;
    }

    if (data_block->data_source != nullptr) {
        free(data_block->data_source);
        data_block->data_source = nullptr;
    }

    if (data_block->signal_rec != nullptr) {
        free(data_block->signal_rec);
        data_block->signal_rec = nullptr;
    }

    if (data_block->signal_desc != nullptr) {
        free(data_block->signal_desc);
        data_block->signal_desc = nullptr;
    }

    if (ddims != nullptr && rank > 0) {
        for (int i = 0; i < rank; i++) {
            if ((cptr = data_block->dims[i].dim) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].synthetic) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].errhi) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].errlo) != nullptr) {
                free(cptr);
            }

            data_block->dims[i].dim = nullptr; // Prevent another Free
            data_block->dims[i].synthetic = nullptr;
            data_block->dims[i].errhi = nullptr;
            data_block->dims[i].errlo = nullptr;

            if ((cptr = (char*)data_block->dims[i].sams) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].offs) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].ints) != nullptr) {
                free(cptr);
            }

            data_block->dims[i].sams = nullptr;
            data_block->dims[i].offs = nullptr;
            data_block->dims[i].ints = nullptr;
        }

        free(ddims);
        data_block->dims = nullptr; // Prevent another Free
    }

    // closeIdamError(&server_block.idamerrorstack);
    free_error_stack(&server_block.idamerrorstack);
    init_data_block(data_block);
    data_block->handle = -1; // Flag this as ready for re-use
}

void udaFreeAll()
{
    // Free All Heap Memory
#ifndef FATCLIENT
    ProtocolId protocol_id;
#endif

#ifndef NOLIBMEMCACHED
    // Free Cache connection object
    uda::cache::free_cache();
#endif

    for (int i = 0; i < getCurrentDataBlockIndex(); ++i) {
#ifndef FATCLIENT
        free_data_block(getDataBlock(i));
#else
        free_data_block(getDataBlock(i));
#endif
    }

    free_data_blocks();

#ifndef FATCLIENT
    g_user_defined_type_list = nullptr; // malloc'd within protocolXML
    g_log_malloc_list = nullptr;
#endif

    close_error();

#ifndef FATCLIENT // <========================== Client Server Code Only

    // After each data request, the server waits for the ClientBlock to be sent.
    // When the client application closes, the socket is closed and the server terminates with a Protocol Error
    // meaning the expected data structure was not received. This error is written to the server log.
    // To avoid this, we can send a ClientBlock with a CLOSEDOWN instruction.

    if (connectionOpen()) {
        client_block.timeout = 0;                                                   // Surrogate CLOSEDOWN instruction
        client_block.clientFlags = client_block.clientFlags | client_flags::CloseDown; // Direct CLOSEDOWN instruction
        protocol_id = ProtocolId::ClientBlock;
        protocol2(*g_client_output, protocol_id, XDRStreamDirection::Send, nullptr, g_log_malloc_list, g_user_defined_type_list,
                  &client_block, protocol_version, g_log_struct_list, *udaPrivateFlags(), UDA_MALLOC_SOURCE_NONE);
        xdrrec_endofrecord(*g_client_output, 1);
    }

#endif // <========================== End of Client Server Code Only

    bool reopen_logs = false;

#ifndef FATCLIENT
    // Close the Socket, XDR Streams and All Files
    closedown(ClosedownType::CLOSE_ALL, nullptr, *g_client_input, *g_client_output, &reopen_logs);
#else
    closedown(ClosedownType::CLOSE_ALL, nullptr, nullptr, nullptr, &reopen_logs);
#endif
}

ServerBlock uda::client::udaGetThreadServerBlock()
{
    return server_block;
}

ClientBlock uda::client::udaGetThreadClientBlock()
{
    return client_block;
}

void uda::client::udaPutThreadServerBlock(ServerBlock* str)
{
    server_block = *str;
}

void uda::client::udaPutThreadClientBlock(ClientBlock* str)
{
    client_block = *str;
}

void uda::client::udaPutServerSocket(int socket)
{
    auto config = client_config();
    config->set("client.socket", socket);
}

int uda::client::udaGetServerSocket()
{
    auto config = client_config();
    return config->get("client.socket");
}

#define UDA_VERSION_STRING_LENGTH 256

void udaGetClientVersionString(char* version_string)
{
    snprintf(version_string, UDA_VERSION_STRING_LENGTH, "%s", UDA_BUILD_VERSION);
}

//! the UDA client library verion number
/**
 * @return the version number
 */
int udaGetClientVersion()
{
    return UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);
}

//! the UDA server version number
/**
 * @return the version number
 */
int udaGetServerVersion()
{
    return server_block.version; // Server Version
}

void udaGetServerVersionString(char* version_string)
{
    int major_version = UDA_GET_MAJOR_VERSION(server_block.version);
    int minor_version = UDA_GET_MINOR_VERSION(server_block.version);
    int bugfix_version = UDA_GET_BUGFIX_VERSION(server_block.version);
    int delta_version = UDA_GET_DELTA_VERSION(server_block.version);
    snprintf(version_string, UDA_VERSION_STRING_LENGTH, "%d.%d.%d.%d", major_version, minor_version, bugfix_version, delta_version);
}

int udaGetClientVersionMajor() { return UDA_VERSION_MAJOR; }
int udaGetClientVersionMinor() { return UDA_VERSION_MINOR; }
int udaGetClientVersionBugfix() { return UDA_VERSION_BUGFIX; }
int udaGetClientVersionDelta() { return UDA_VERSION_DELTA; }

int udaGetServerVersionMajor() { return UDA_GET_MAJOR_VERSION(server_block.version); }
int udaGetServerVersionMinor() { return UDA_GET_MINOR_VERSION(server_block.version); }
int udaGetServerVersionBugfix() { return UDA_GET_BUGFIX_VERSION(server_block.version); }
int udaGetServerVersionDelta() { return UDA_GET_DELTA_VERSION(server_block.version); }

//! the UDA server error code returned
/**
 * @return the error code
 */
int udaGetServerErrorCode()
{
    return server_block.error; // Server Error Code
}

//! the UDA server error message returned
/**
 * @return the error message
 */
const char* udaGetServerErrorMsg()
{
    return server_block.msg; // Server Error Message
}

//! the number of UDA server error message records returned in the error stack
/**
 * @return the number of records
 */
int udaGetServerErrorStackSize()
{
    return server_block.idamerrorstack.nerrors; // Server Error Stack Size (No.Records)
}

//! the Type of server error of a specific server error record
/**
 * @param record the error stack record number
 * @return the type id
 */
int udaGetServerErrorStackRecordType(int record)
{
    if (record < 0 || (unsigned int)record >= server_block.idamerrorstack.nerrors) {
        return 0;
    }
    return (int)server_block.error_stack[record].type; // Server Error Stack Record Type
}

//! the Error code of a specific server error record
/**
 * @param record the error stack record number
 * @return the error code
 */
int udaGetServerErrorStackRecordCode(int record)
{
    if (record < 0 || (unsigned int)record >= server_block.idamerrorstack.nerrors) {
        return 0;
    }
    return server_block.error_stack[record].code; // Server Error Stack Record Code
}

//! the Server error Location name of a specific error record
/**
 * @param record the error stack record number
 * @return the location name
 */
const char* udaGetServerErrorStackRecordLocation(int record)
{
    if (record < 0 || (unsigned int)record >= server_block.idamerrorstack.nerrors) {
        return nullptr;
    }
    return server_block.error_stack[record].location; // Server Error Stack Record Location
}

//! the Server error message of a specific error record
/**
 * @param record the error stack record number
 * @return the error message
 */
const char* udaGetServerErrorStackRecordMsg(int record)
{
    UDA_LOG(UDA_LOG_DEBUG, "record {}", record);
    UDA_LOG(UDA_LOG_DEBUG, "count  {}", server_block.idamerrorstack.nerrors);
    if (record < 0 || (unsigned int)record >= server_block.idamerrorstack.nerrors) {
        return nullptr;
    }
    return server_block.error_stack[record].msg; // Server Error Stack Record Message
}
