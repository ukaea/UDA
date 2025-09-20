#include "uda_plugin.h"

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <optional>

#include <client/accAPI.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <client/udaGetAPI.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>
#include <client/udaClient.h>
#include <version.h>

#include <fmt/format.h>

#if !defined(__GNUC__)
#  define strcasecmp _stricmp
#endif

class UdaPlugin {
    public:
    UdaPlugin() {
        strcpy(old_server_host_, getIdamServerHost());
    }

    void init(IDAM_PLUGIN_INTERFACE* plugin_interface) {
        REQUEST_DATA* request = plugin_interface->request_data;
        if (!init_ || STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {

            udaResetProperties();

            // Hand over Server IO File Handles to UDA Client library
            UDA_LOG(UDA_LOG_DEBUG, "Handing over Server File Handles to UDA Client\n");

            const char* cache = getenv("UDA_UDA_PLUGIN_CLIENT_CACHE");
            cache_enabled_ = (cache == nullptr) or (std::stoi(cache) > 0);
            init_ = true;
        }
    }
    void reset(IDAM_PLUGIN_INTERFACE* plugin_interface) {
        if (!init_) {
            // Not previously initialised: Nothing to do!
            return;
        }
        udaResetProperties();
        udaFreeAll();

        putIdamServerHost(old_server_host_);    // Original Host
        putIdamServerPort(old_server_port_);    // Original Port
                                                // Free Heap & reset counters
        init_ = false;
    }

    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int get(IDAM_PLUGIN_INTERFACE* plugin_interface);

    private:
    std::string make_cache_key(std::string_view signal, std::string_view source, std::string_view host, int port);
    std::optional<int> check_cache(const std::string& key);

    bool init_ = false;

    char old_server_host_[STRING_LENGTH];
    int old_server_port_ = getIdamServerPort();

    std::unordered_map<std::string, int> cache_ = {};
    bool cache_enabled_ = true;
};

extern int UDAPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    try {
        static UdaPlugin plugin = {};

        //----------------------------------------------------------------------------------------
        // Standard v1 Plugin Interface

        if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
            UDA_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
            UDA_THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        }

        plugin_interface->pluginVersion = THISPLUGIN_VERSION;
        auto housekeeping = plugin_interface->housekeeping;
        REQUEST_DATA* request = plugin_interface->request_data;

        if (housekeeping || STR_IEQUALS(request->function, "reset")) {
            plugin.reset(plugin_interface);
            return 0;
        }
        if (STR_IEQUALS(request->function, "create") || STR_IEQUALS(request->function, "initialise")) {
            plugin.init(plugin_interface);
            return 0;
        }

        //----------------------------------------------------------------------------------------
        // Plugin Functions
        //----------------------------------------------------------------------------------------

        if (STR_IEQUALS(request->function, "help")) {
            return plugin.help(plugin_interface);
        } else if (STR_IEQUALS(request->function, "version")) {
            return plugin.version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "builddate")) {
            return plugin.build_date(plugin_interface);
        } else if (STR_IEQUALS(request->function, "defaultmethod")) {
            return plugin.default_method(plugin_interface);
        } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
            return plugin.max_interface_version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "get")) {
            return plugin.get(plugin_interface);
        } else {
           RAISE_PLUGIN_ERROR_AND_EXIT("unkown function requested", plugin_interface) 
        }
    } catch (const std::exception& ex) {
        RAISE_PLUGIN_ERROR_AND_EXIT(ex.what(), plugin_interface) 
    }
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int UdaPlugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "\nUDA: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "UDA: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param plugin_interface
 * @return
 */
int UdaPlugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, UDA_BUILD_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int UdaPlugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int UdaPlugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int UdaPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                  "Maximum Interface Version");
}

std::string UdaPlugin::make_cache_key(std::string_view signal, std::string_view source, std::string_view host, int port)
{
    if (!cache_enabled_) {
        return {};
    }
    // start with signal as most likely to change most frequently
    return fmt::format("{}|{}|{}|{}", signal, source, host, port);
}

std::optional<int> UdaPlugin::check_cache(const std::string& key)
{
    if (!cache_enabled_) {
        return std::nullopt;
    }

    auto result = cache_.find(key);
    if (result != cache_.end()) {
        return result->second;
    }
    return std::nullopt;
}

int UdaPlugin::get(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    int err = 0;

    // Version 6 structures

    typedef struct OldClientBlock {

        int version;
        int pid;                    // Client Application process id
        char uid[STRING_LENGTH];    // Who the Client is

        // Server properties set by the client

        int timeout;                // Server Shutdown after this time (minutes) if no data request
        int compressDim;            // Enable Compression of the Dimensional Data?

        unsigned int clientFlags;   // client defined properties passed via bit flags
        int altRank;                // Specify the rank of the alternative signal/source to be used

        int get_nodimdata;          // Don't send Dimensional Data: Send an index only.
        int get_timedble;           // Return Time Dimension Data in Double Precision if originally compressed
        int get_dimdble;            // Return all Dimensional Data in Double Precision
        int get_datadble;           // Return Data in Double Precision

        int get_bad;                // Return Only Data with Bad Status value
        int get_meta;               // Return Meta Data associated with Signal
        int get_asis;               // Return data as Stored in data Archive
        int get_uncal;              // Disable Calibration Correction
        int get_notoff;             // Disable Timing Offset Correction
        int get_scalar;             // Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
        int get_bytes;              // Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

        unsigned int privateFlags;  // set of private flags used to communicate server to server

    } OLD_CLIENT_BLOCK;

    typedef struct OldDataBlock {
        int handle;
        int errcode;
        int source_status;
        int signal_status;
        int rank;
        int order;
        int data_type;

        int error_type;
        int error_model;                // Identify the Error Model
        int errasymmetry;               // Flags whether or not error data are asymmetrical
        int error_param_n;              // the Number of Model Parameters

        int data_n;
        char* data;
        char* synthetic;                // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

        char* errhi;                    // Error Array (Errors above the line: data + error)
        char* errlo;                    // Error Array (Errors below the line: data - error)
        float errparams[MAXERRPARAMS];  // the array of model parameters

        char data_units[STRING_LENGTH];
        char data_label[STRING_LENGTH];
        char data_desc[STRING_LENGTH];

        char error_msg[STRING_LENGTH];

        DIMS* dims;
        DATA_SYSTEM* data_system;
        SYSTEM_CONFIG* system_config;
        DATA_SOURCE* data_source;
        SIGNAL* signal_rec;
        SIGNAL_DESC* signal_desc;

        OLD_CLIENT_BLOCK client_block;  // Used to pass properties into legacy data reader plugins - ignore!

        int opaque_type;                // Identifies the Data Structure Type;
        int opaque_count;               // Number of Instances of the Data Structure;
        void* opaque_block;             // Opaque pointer to Hierarchical Data Structures
    } OLD_DATA_BLOCK;

    int handle = 0;
    int newPort = 0;

/* ----------------------------------------------------------------------
Notes: there are three pathways depending on the request pattern

1> request is the result of an SQL query against the metadata catalog
    if the primary key fields of the data_source and signal_desc structures are non zero
2> request is a device redirect or a server protocol
    path = server:port/source, server=""
3> request is a function call
    path = "", name-value pair arguments, server=""
4> request is a server call
    server=server:port, path=source
*/

// Identify execution pathway

    int pathway = 0;

    DATA_SOURCE* data_source = plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = plugin_interface->signal_desc;
    REQUEST_DATA* request = plugin_interface->request_data;

    if (data_source->source_id > 0 && signal_desc->signal_desc_id > 0) {
        pathway = 1;
    } else if (request->path[0] != '\0' && request->server[0] == '\0') {
        pathway = 2;
    } else if (request->path[0] == '\0' && request->server[0] == '\0') {
        pathway = 3;
    } else if (request->server[0] != '\0') {
        // Source URL may be an nullptr string
        pathway = 4;
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Execution pathway not recognised: Unable to execute the request!\n");
        UDA_THROW_ERROR(999, "Execution pathway not recognised: Unable to execute the request!");
    }

    //----------------------------------------------------------------------
    // Private Flags, User Specified Flags and Properties for the Remote Server

    resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
    // Ensure Hierarchical Data are passed as an opaque object/file
    setIdamPrivateFlag(PRIVATEFLAG_XDRFILE);

    // This fails if the legacy UDA plugin is called by a server in the forward chain and it set marked a 'private'
    // For IMAS development, this has been disabled
    //if(environment.external_user) setIdamPrivateFlag(PRIVATEFLAG_EXTERNAL);    // Maintain external user status

    // Set Userid

    // ... to be implemented

    // Set Properties

    CLIENT_BLOCK* client_block = plugin_interface->client_block;

    if (client_block->get_nodimdata) udaSetProperty("get_nodimdata");
    if (client_block->get_timedble) udaSetProperty("get_timedble");
    if (client_block->get_dimdble) udaSetProperty("get_dimdble");
    if (client_block->get_datadble) udaSetProperty("get_datadble");

    if (client_block->get_bad) udaSetProperty("get_bad");
    if (client_block->get_meta) udaSetProperty("get_meta");
    if (client_block->get_asis) udaSetProperty("get_asis");
    if (client_block->get_uncal) udaSetProperty("get_uncal");
    if (client_block->get_notoff) udaSetProperty("get_notoff");
    if (client_block->get_scalar) udaSetProperty("get_scalar");
    if (client_block->get_bytes) udaSetProperty("get_bytes");

    // Timeout ...

    // AltRank ...

    // Client Flags ...

    udaResetClientFlag((unsigned int)CLIENTFLAG_FULLRESET);
    udaSetClientFlag(client_block->clientFlags);

    // Client application provenance

    //putIdamClientDOI(client_block->DOI);
    //putIdamClientOSName(client_block->OSName);

    // Client authentication x509 certificate

    //----------------------------------------------------------------------
    // Signal/Data Object & Data Source Details from the UDA Database records

    // Very primitive: Need to replicate fully the idamGetAPI arguments from the database

    std::string signal;
    std::string source;
    std::string host = old_server_host_;
    int port = old_server_port_;

    if (pathway == 1) {    // Request via the Database

        signal = fmt::format("{}::{}", data_source->archive, signal_desc->signal_name);
        source = fmt::format("{}::{}", data_source->device_name, data_source->exp_number);

        if (data_source->server[0] != '\0') {
            char* p = nullptr, * s = nullptr;
            if ((s = strstr(data_source->server, "SSL://")) != nullptr) {
                if ((p = strstr(s + 6, ":")) == nullptr) {
                    // look for a port number in the server name
                    p = strstr(s + 6, " ");
                }
            } else {
                if ((p = strstr(data_source->server, ":")) == nullptr) {
                    // look for a port number in the server name
                    p = strstr(data_source->server, " ");
                }
            }

            if (p != nullptr) {
                p[0] = '\0';
                if (strcasecmp(old_server_host_, data_source->server) != 0) {
                    strcpy(old_server_host_, data_source->server);
                    putIdamServerHost(data_source->server);
                    host = data_source->server;
                }
                if (IsNumber(&p[1])) {
                    newPort = atoi(&p[1]);
                    if (newPort != old_server_port_) {
                        putIdamServerPort(newPort);
                        port = newPort;
                        old_server_port_ = newPort;
                    }
                } else {
                    UDA_THROW_ERROR(999,
                                    "The Server Port must be an Integer Number passed using the formats 'server:port' or 'server port'");
                }
            } else {
                if (strcasecmp(old_server_host_, data_source->server) != 0) {
                    strcpy(old_server_host_, data_source->server);
                    putIdamServerHost(data_source->server);
                }
            }
        } else {
            UDA_THROW_ERROR(999, "No Server has been specified!");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", data_source->server);
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", newPort);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Database based Request)\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source.c_str());

    } else if (pathway == 2) {

        char _source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Device redirect or server protocol

        strcpy(request->server, request->path);        // Extract the Server Name and Port
        char* p = nullptr, * s = nullptr;

        if ((s = strstr(data_source->server, "SSL://")) != nullptr) {
            if ((p = strchr(s + 6, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';                            // Break the String (work)
                strcpy(_source, p + 1);                // Extract the Source URL Argument
            } else {
                UDA_THROW_ERROR(999,
                                "The Remote Server Data Source specified does not comply with the naming model: serverHost:port/sourceURL");
            }
        } else {
            if ((p = strchr(request->server, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';                            // Break the String (work)
                strcpy(_source, p + 1);                // Extract the Source URL Argument
            } else {
                UDA_THROW_ERROR(999,
                                "The Remote Server Data Source specified does not comply with the naming model: serverHost:port/sourceURL");
            }
        }

        if ((s = strstr(request->server, "SSL://")) != nullptr) {
            if ((p = strstr(s + 6, ":")) == nullptr) {
                // look for a port number in the server name skipping SSL:// prefix
                p = strstr(s + 6, " ");
            }
        } else {
            if ((p = strstr(request->server, ":")) == nullptr) {
                // look for a port number in the server name skipping SSL:// prefix
                p = strstr(request->server, " ");
            }
        }

        if (p != nullptr) {
            p[0] = '\0';
            if (strcasecmp(old_server_host_, request->server) != 0) {
                strcpy(old_server_host_, request->server);
                putIdamServerHost(request->server);    // different host name?
                host = request->server;
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != old_server_port_) {
                    putIdamServerPort(newPort);
                    old_server_port_ = newPort;
                    port = newPort;
                }
            } else {
                UDA_THROW_ERROR(999,
                                "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server port'");
            }
        } else {
            if (strcasecmp(old_server_host_, request->server) != 0) {
                strcpy(old_server_host_, request->server);
                putIdamServerHost(request->server);
                host = request->server;
            }
        }

        // handle = idamGetAPI(request->signal, _source);
        signal = request->signal;
        source = _source;

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", host.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", port);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Device redirect or server protocol based Request)\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source.c_str());

    } else if (pathway == 3) {

        //----------------------------------------------------------------------
        // Function library

        const char* _host = nullptr;
        bool isHost = findStringValue(&request->nameValueList, &_host, "host");

        const char* _signal = nullptr;
        bool isSignal = findStringValue(&request->nameValueList, &_signal, "signal");

        const char* _source = nullptr;
        bool isSource = findStringValue(&request->nameValueList, &_source, "source");

        bool isPort = findIntValue(&request->nameValueList, &newPort, "port");

        // Set host and port

        if (isHost && strcasecmp(old_server_host_, _host) != 0) {
            strcpy(old_server_host_, _host);
            putIdamServerHost(_host);
            host = _host;
        }
        if (isPort && old_server_port_ != newPort) {
            old_server_port_ = newPort;
            putIdamServerPort(newPort);
            port = newPort;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", host.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", port);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (plugin library method based Request)\n");

        if (isSignal && isSource) {
            signal = _signal;
            UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal.c_str());
            source = _source;
            UDA_LOG(UDA_LOG_DEBUG, "idamAPIPlugin; Source: %s\n", source.c_str());
            // handle = idamGetAPI(signal, source);
        } else if (isSignal) {
            signal = _signal;
            UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal.c_str());
            source = request->source;
            UDA_LOG(UDA_LOG_DEBUG, "idamAPIPlugin; Source: %s\n", source.c_str());
            // handle = idamGetAPI(signal, request->source);
        } else {
            UDA_THROW_ERROR(999, "A data object (signal) has not been specified!");
        }
    } else if (pathway == 4) {

        char _source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Server protocol

        strcpy(_source, request->file);                // The Source URL Argument

        char* p = nullptr;
        char* s = nullptr;

        if ((s = strstr(request->server, "SSL://")) != nullptr) {
            if ((p = strstr(s + 6, ":")) == nullptr) {
                // look for a port number in the server name
                p = strstr(s + 6, " ");
            }
        } else {
            if ((p = strstr(request->server, ":")) == nullptr) {
                // look for a port number in the server name
                p = strstr(request->server, " ");
            }
        }

        if (p != nullptr) {                            // look for a port number in the server name
            p[0] = '\0';                        // Split
            if (strcasecmp(old_server_host_, request->server) != 0) {    // Different Hosts?
                strcpy(old_server_host_, request->server);        // Preserve
                putIdamServerHost(request->server);        // Change to a different host name
                host = request->server;
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != old_server_port_) {
                    // Different Ports?
                    putIdamServerPort(newPort);
                    old_server_port_ = newPort;
                    port = newPort;
                }
            } else {
                UDA_THROW_ERROR(999,
                                "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server port'");
            }
        } else {
            // No port number passed
            if (strcasecmp(old_server_host_, request->server) != 0) {    // Different Hosts?
                strcpy(old_server_host_, request->server);
                putIdamServerHost(request->server);
                host = request->server;
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "UDA Server Host for UDA Plugin %s\n", host.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "UDA Server Port for UDA Plugin %d\n", port);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Server protocol based Request)\n");
        signal = request->signal;
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal.c_str());
        source = _source;
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source.c_str());

        // handle = idamGetAPI(request->signal, source);
    }

    auto cache_key = make_cache_key(signal, source, host, port);
    auto maybe_result = check_cache(cache_key);
    if (maybe_result.has_value())
    {
        handle = maybe_result.value();
    } else {
        handle = idamGetAPI(signal.c_str(), source.c_str());
        cache_.insert({cache_key, handle});
    }

    resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
    udaResetClientFlag((unsigned int)CLIENTFLAG_FULLRESET);

    //----------------------------------------------------------------------
    // Test for Errors: Close Socket and Free heap

    UDA_LOG(UDA_LOG_DEBUG, "Returned from idamGetAPI API: handle = %d, error code = %d\n", handle,
            getIdamErrorCode(handle));

    if (handle < 0) {
        UDA_THROW_ERROR(abs(handle), getIdamServerErrorStackRecordMsg(0));
    } else if ((err = getIdamErrorCode(handle)) != 0) {
        UDA_THROW_ERROR(err, getIdamErrorMsg(handle));
    }

    //----------------------------------------------------------------------
    // Copy the Data Block - method is version dependent
    // All xml defined actions on the data will have been executed
    // data_source & signal_desc records may will be different to those handed in across the interface
    // 1> handed in: records how this plugin was identified
    // 2> handed out: records what data were returned
    // In the provenance context 2> is more important (only one record pair can be returned to the application)

    // Check the originals have no XML action definitions before replacement
    // Why should a plugin have this concern?

    DATA_BLOCK* data_block = plugin_interface->data_block;

    if (getIdamClientVersion() >= 7) {
        // This should contain everything!
        *data_block = *getIdamDataBlock(handle);
    } else {                        // use abstraction functions

        // Straight structure mapping causes potential problems when the client library uses different versions
        // of the DATA_BLOCK structure
        // or its component structure CLIENT_BLOCK
        // or the initialisation is incomplete!
        // Write the structure components element by element! (Ignore the CLIENT_BLOCK component)

        DATA_BLOCK db;
        initDataBlock(&db);

        auto odb = (OLD_DATA_BLOCK*)getIdamDataBlock(handle);

        db.handle = odb->handle;
        db.errcode = odb->errcode;
        db.source_status = odb->source_status;
        db.signal_status = odb->signal_status;
        db.rank = odb->rank;
        db.order = odb->order;
        db.data_type = odb->data_type;

        db.error_type = odb->error_type;
        db.error_model = odb->error_model;
        db.errasymmetry = odb->errasymmetry;
        db.error_param_n = odb->error_param_n;

        db.data_n = odb->data_n;
        db.data = odb->data;
        db.synthetic = odb->synthetic;

        db.errhi = odb->errhi;
        db.errlo = odb->errlo;

        for (int i = 0; i < MAXERRPARAMS; i++) {
            db.errparams[i] = odb->errparams[i];
        }

        strcpy(db.data_units, odb->data_units);
        strcpy(db.data_label, odb->data_label);
        strcpy(db.data_desc, odb->data_desc);
        strcpy(db.error_msg, odb->error_msg);

        db.dims = odb->dims;        // These have not changed between versions 6 & 7
        db.data_system = odb->data_system;
        db.system_config = odb->system_config;
        db.data_source = odb->data_source;
        db.signal_rec = odb->signal_rec;
        db.signal_desc = odb->signal_desc;

        db.opaque_type = odb->opaque_type;
        db.opaque_count = odb->opaque_count;
        db.opaque_block = odb->opaque_block;

        *data_block = db;
    }

    // Replace provenance metadata records

    if (client_block->get_meta && strlen(data_source->xml) == 0 && strlen(signal_desc->xml) == 0) {
        *data_source = *(data_block->data_source);
        *signal_desc = *(data_block->signal_desc);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Exit\n");

    //----------------------------------------------------------------------
    // If the Data are Hierarchical, then necessary to forward the xdr file
    //
    // List of structures from external server may conflict with local definitions
    //    Don't use local definitions
    //
    // No access to malloc log within client
    //
    // Data received is a Data Tree. This would need to be restructured - i.e., pointer extracted from
    // structure SARRAY (may be different to local SARRAY!)
    //    Don't pass a data tree - use an XDR file instead.
    //    Required if http is to be adopted as middleware protocol
    //     Relay everything from the external server back to the client without interpretation.
    //
    // Namespace issues: Both the Client and the Server use the same functions to Query. The PRE_LOAD requirement of MDS+
    // causes the UDA client library to be loaded ahead of the server library: Result confusion and seg fault errors.
    // Need to add unique name component to UDA client server to provide namespace separation.
    // Prepare a reduced set of external symbols for the client library attached to the server!

    // Structured data are best served as serialised objects - no messy heap logs to free
    // Data should be freed with a 'reset' method call after the data have been transmitted
    // Add a new 'freeImmediate' plugin property that causes a heap free without other state changes, e.g. socket


    // Problem tests: 115, 118, 124, 141, 142

    //----------------------------------------------------------------------

    return err;
}

