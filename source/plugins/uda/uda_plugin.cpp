#include "uda_plugin.h"

#include <stdlib.h>

#include <client/accAPI.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <client/udaGetAPI.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>
#include <client/udaClient.h>

#if !defined(__GNUC__)
#  define strcasecmp _stricmp
#endif

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, char* oldServerHost, int* oldPort);

extern int UDAPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;
    static char oldServerHost[STRING_LENGTH] = "";
    static int oldPort = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        UDA_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    housekeeping = idam_plugin_interface->housekeeping;

    REQUEST_DATA* request = idam_plugin_interface->request_data;

    if (housekeeping || STR_IEQUALS(request->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Resetting all UDA client properties

        resetIdamProperties();

        putIdamServerHost(oldServerHost);    // Original Host
        putIdamServerPort(oldPort);    // Original Port

        // Free Heap & reset counters

        init = 0;

        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {

        // Default Server Host and Port

        strcpy(oldServerHost, getIdamServerHost());    // Current Host
        oldPort = getIdamServerPort();            // Current Port

        // Resetting all UDA client properties

        resetIdamProperties();

        // Hand over Server IO File Handles to UDA Client library

        UDA_LOG(UDA_LOG_DEBUG, "Handing over Server File Handles to UDA Client\n");

        init = 1;
        if (STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (STR_IEQUALS(request->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "get")) {
        err = do_get(idam_plugin_interface, oldServerHost, &oldPort);
    } else {
        THROW_ERROR(999, "Unknown function requested!");
    }

    //--------------------------------------------------------------------------------------
    // Housekeeping

    return err;
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nUDA: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "UDA: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                  "Maximum Interface Version");
}

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, char* oldServerHost, int* oldPort)
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

    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    REQUEST_DATA* request = idam_plugin_interface->request_data;

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
        THROW_ERROR(999, "Execution pathway not recognised: Unable to execute the request!");
    }

    //----------------------------------------------------------------------
    // Private Flags, User Specified Flags and Properties for the Remote Server

    resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
    // Ensure Hierarchical Data are passed as an opaque object/file
    setIdamPrivateFlag(PRIVATEFLAG_XDRFILE);

    // This fails if the legacy UDA plugin is called by a server in the forward chain and it set marked a 'private'
    // For IMAS development, this has been disabled
    //if(environment.external_user) setIdamPrivateFlag(PRIVATEFLAG_EXTERNAL);	// Maintain external user status

    // Set Userid

    // ... to be implemented

    // Set Properties

    CLIENT_BLOCK* client_block = idam_plugin_interface->client_block;

    if (client_block->get_nodimdata) setIdamProperty("get_nodimdata");
    if (client_block->get_timedble) setIdamProperty("get_timedble");
    if (client_block->get_dimdble) setIdamProperty("get_dimdble");
    if (client_block->get_datadble) setIdamProperty("get_datadble");

    if (client_block->get_bad) setIdamProperty("get_bad");
    if (client_block->get_meta) setIdamProperty("get_meta");
    if (client_block->get_asis) setIdamProperty("get_asis");
    if (client_block->get_uncal) setIdamProperty("get_uncal");
    if (client_block->get_notoff) setIdamProperty("get_notoff");
    if (client_block->get_scalar) setIdamProperty("get_scalar");
    if (client_block->get_bytes) setIdamProperty("get_bytes");

    // Timeout ...

    // AltRank ...

    // Client Flags ...

    resetIdamClientFlag((unsigned)CLIENTFLAG_FULLRESET);
    setIdamClientFlag(client_block->clientFlags);

    // Client application provenance

    //putIdamClientDOI(client_block->DOI);
    //putIdamClientOSName(client_block->OSName);

    // Client authentication x509 certificate

    //----------------------------------------------------------------------
    // Signal/Data Object & Data Source Details from the UDA Database records

    // Very primitive: Need to replicate fully the idamGetAPI arguments from the database

    if (pathway == 1) {    // Request via the Database

        const auto lstr = 2 * MAXNAME + 2;
        char signal[lstr];
        char source[lstr];

        snprintf(signal, lstr, "%s::%s", data_source->archive, signal_desc->signal_name);
        snprintf(source, lstr, "%s::%d", data_source->device_name, data_source->exp_number);

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
                if (strcasecmp(oldServerHost, data_source->server) != 0) {
                    strcpy(oldServerHost, data_source->server);
                    putIdamServerHost(data_source->server);
                }
                if (IsNumber(&p[1])) {
                    newPort = atoi(&p[1]);
                    if (newPort != *oldPort) {
                        putIdamServerPort(newPort);
                        *oldPort = newPort;
                    }
                } else {
                    THROW_ERROR(999,
                                "The Server Port must be an Integer Number passed using the formats 'server:port' or 'server port'");
                }
            } else {
                if (strcasecmp(oldServerHost, data_source->server) != 0) {
                    strcpy(oldServerHost, data_source->server);
                    putIdamServerHost(data_source->server);
                }
            }
        } else {
            THROW_ERROR(999, "No Server has been specified!");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", data_source->server);
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", newPort);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Database based Request)\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal);
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source);

        handle = idamGetAPI(signal, source);

    } else if (pathway == 2) {

        char source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Device redirect or server protocol

        strcpy(request->server, request->path);        // Extract the Server Name and Port
        char* p = nullptr, * s = nullptr;

        if ((s = strstr(data_source->server, "SSL://")) != nullptr) {
            if ((p = strchr(s + 6, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';                            // Break the String (work)
                strcpy(source, p + 1);                // Extract the Source URL Argument
            } else {
                THROW_ERROR(999,
                            "The Remote Server Data Source specified does not comply with the naming model: serverHost:port/sourceURL");
            }
        } else {
            if ((p = strchr(request->server, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';                            // Break the String (work)
                strcpy(source, p + 1);                // Extract the Source URL Argument
            } else {
                THROW_ERROR(999,
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
            if (strcasecmp(oldServerHost, request->server) != 0) {
                strcpy(oldServerHost, request->server);
                putIdamServerHost(request->server);    // different host name?
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != *oldPort) {
                    putIdamServerPort(newPort);
                    *oldPort = newPort;
                }
            } else {
                THROW_ERROR(999,
                            "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server port'");
            }
        } else {
            if (strcasecmp(oldServerHost, request->server) != 0) {
                strcpy(oldServerHost, request->server);
                putIdamServerHost(request->server);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", request->server);
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", newPort);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Device redirect or server protocol based Request)\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", request->signal);
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source);

        handle = idamGetAPI(request->signal, source);

    } else if (pathway == 3) {

        //----------------------------------------------------------------------
        // Function library

        const char* host = nullptr;
        bool isHost = findStringValue(&request->nameValueList, &host, "host");

        const char* signal = nullptr;
        bool isSignal = findStringValue(&request->nameValueList, &signal, "signal");

        const char* source = nullptr;
        bool isSource = findStringValue(&request->nameValueList, &source, "source");

        bool isPort = findIntValue(&request->nameValueList, &newPort, "port");

        // Set host and port

        if (isHost && strcasecmp(oldServerHost, host) != 0) {
            strcpy(oldServerHost, host);
            putIdamServerHost(host);
        }
        if (isPort && *oldPort != newPort) {
            *oldPort = newPort;
            putIdamServerPort(newPort);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Host for Idam Plugin %s\n", host);
        UDA_LOG(UDA_LOG_DEBUG, "Idam Server Port for Idam Plugin %d\n", newPort);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (plugin library method based Request)\n");

        if (isSignal && isSource) {
            UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal);
            UDA_LOG(UDA_LOG_DEBUG, "idamAPIPlugin; Source: %s\n", source);
            handle = idamGetAPI(signal, source);
        } else if (isSignal) {
            UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", signal);
            UDA_LOG(UDA_LOG_DEBUG, "idamAPIPlugin; Source: %s\n", request->source);
            handle = idamGetAPI(signal, request->source);
        } else {
            THROW_ERROR(999, "A data object (signal) has not been specified!");
        }
    } else if (pathway == 4) {

        char source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Server protocol

        strcpy(source, request->file);                // The Source URL Argument

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
            if (strcasecmp(oldServerHost, request->server) != 0) {    // Different Hosts?
                strcpy(oldServerHost, request->server);        // Preserve
                putIdamServerHost(request->server);        // Change to a different host name
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != *oldPort) {
                    // Different Ports?
                    putIdamServerPort(newPort);
                    *oldPort = newPort;
                }
            } else {
                THROW_ERROR(999,
                            "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server port'");
            }
        } else {
            // No port number passed
            if (strcasecmp(oldServerHost, request->server) != 0) {    // Different Hosts?
                strcpy(oldServerHost, request->server);
                putIdamServerHost(request->server);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "UDA Server Host for UDA Plugin %s\n", request->server);
        UDA_LOG(UDA_LOG_DEBUG, "UDA Server Port for UDA Plugin %d\n", newPort);
        UDA_LOG(UDA_LOG_DEBUG, "Calling idamGetAPI API (Server protocol based Request)\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal: %s\n", request->signal);
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", source);

        handle = idamGetAPI(request->signal, source);
    }

    resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
    resetIdamClientFlag((unsigned)CLIENTFLAG_FULLRESET);

    //----------------------------------------------------------------------
    // Test for Errors: Close Socket and Free heap

    UDA_LOG(UDA_LOG_DEBUG, "Returned from idamGetAPI API: handle = %d, error code = %d\n", handle,
            getIdamErrorCode(handle));

    if (handle < 0) {
        THROW_ERROR(abs(handle), getIdamServerErrorStackRecordMsg(0));
    } else if ((err = getIdamErrorCode(handle)) != 0) {
        THROW_ERROR(err, getIdamErrorMsg(handle));
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

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

