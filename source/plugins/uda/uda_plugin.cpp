#include "uda_plugin.h"

#include <cstdlib>

#include "client/udaClient.h"
#include "clientserver/initStructs.h"
#include "clientserver/stringUtils.h"
#include "include/uda/uda_plugin_base.hpp"
#include "logging/logging.h"
#include "server/serverPlugin.h"
#include "uda/client.h"

#include <boost/filesystem.hpp>
#include <fmt/format.h>

using namespace uda::client_server;
using namespace uda::client;

namespace uda::plugins::uda
{

class Plugin : public UDAPluginBase
{
  public:
    Plugin();

    int get(UDA_PLUGIN_INTERFACE* plugin_interface);

    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override
    {
        old_host_ = udaGetServerHost();
        old_port_ = udaGetServerPort();
    }

    void reset() override {}

  private:
    std::string old_host_;
    int old_port_;
};

Plugin::Plugin()
    : UDAPluginBase("UDA", 1, "get", boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("get", static_cast<UDAPluginBase::plugin_member_type>(&Plugin::get));
}

} // namespace uda::plugins::uda

extern int UDAPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static uda::plugins::uda::Plugin plugin = {};
    return plugin.call(plugin_interface);
}

int uda::plugins::uda::Plugin::get(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int err = 0;

    // Version 6 structures

    typedef struct OldClientBlock {

        int version;
        int pid;                 // Client Application process id
        char uid[STRING_LENGTH]; // Who the Client is

        // Server properties set by the client

        int timeout;     // Server Shutdown after this time (minutes) if no data request
        int compressDim; // Enable Compression of the Dimensional Data?

        unsigned int clientFlags; // client defined properties passed via bit flags
        int altRank;              // Specify the rank of the alternative signal/source to be used

        int get_nodimdata; // Don't send Dimensional Data: Send an index only.
        int get_timedble;  // Return Time Dimension Data in Double Precision if originally compressed
        int get_dimdble;   // Return all Dimensional Data in Double Precision
        int get_datadble;  // Return Data in Double Precision

        int get_bad;    // Return Only Data with Bad Status value
        int get_meta;   // Return Meta Data associated with Signal
        int get_asis;   // Return data as Stored in data Archive
        int get_uncal;  // Disable Calibration Correction
        int get_notoff; // Disable Timing Offset Correction
        int get_scalar; // Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
        int get_bytes;  // Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

        unsigned int privateFlags; // set of private flags used to communicate server to server

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
        int error_model;   // Identify the Error Model
        int errasymmetry;  // Flags whether or not error data are asymmetrical
        int error_param_n; // the Number of Model Parameters

        int data_n;
        char* data;
        char* synthetic; // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

        char* errhi;                   // Error Array (Errors above the line: data + error)
        char* errlo;                   // Error Array (Errors below the line: data - error)
        float errparams[MAXERRPARAMS]; // the array of model parameters

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

        OLD_CLIENT_BLOCK client_block; // Used to pass properties into legacy data reader plugins - ignore!

        int opaque_type;    // Identifies the Data Structure Type;
        int opaque_count;   // Number of Instances of the Data Structure;
        void* opaque_block; // Opaque pointer to Hierarchical Data Structures
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
        error(plugin_interface, "Execution pathway not recognised: Unable to execute the request!");
    }

    //----------------------------------------------------------------------
    // Private Flags, User Specified Flags and Properties for the Remote Server

    udaResetPrivateFlag(PRIVATEFLAG_FULLRESET);
    // Ensure Hierarchical Data are passed as an opaque object/file
    udaSetPrivateFlag(PRIVATEFLAG_XDRFILE);

    // This fails if the legacy UDA plugin is called by a server in the forward chain and it set marked a 'private'
    // For IMAS development, this has been disabled
    // if(environment.external_user) udaSetPrivateFlag(PRIVATEFLAG_EXTERNAL);    // Maintain external user status

    // Set Userid

    // ... to be implemented

    // Set Properties

    CLIENT_BLOCK* client_block = plugin_interface->client_block;

    if (client_block->get_nodimdata) {
        udaSetProperty("get_nodimdata");
    }
    if (client_block->get_timedble) {
        udaSetProperty("get_timedble");
    }
    if (client_block->get_dimdble) {
        udaSetProperty("get_dimdble");
    }
    if (client_block->get_datadble) {
        udaSetProperty("get_datadble");
    }

    if (client_block->get_bad) {
        udaSetProperty("get_bad");
    }
    if (client_block->get_meta) {
        udaSetProperty("get_meta");
    }
    if (client_block->get_asis) {
        udaSetProperty("get_asis");
    }
    if (client_block->get_uncal) {
        udaSetProperty("get_uncal");
    }
    if (client_block->get_notoff) {
        udaSetProperty("get_notoff");
    }
    if (client_block->get_scalar) {
        udaSetProperty("get_scalar");
    }
    if (client_block->get_bytes) {
        udaSetProperty("get_bytes");
    }

    // Timeout ...

    // AltRank ...

    // Client Flags ...

    udaResetClientFlag(CLIENTFLAG_FULLRESET);
    udaSetClientFlag(client_block->clientFlags);

    // Client application provenance

    // putIdamClientDOI(client_block->DOI);
    // putIdamClientOSName(client_block->OSName);

    // Client authentication x509 certificate

    //----------------------------------------------------------------------
    // Signal/Data Object & Data Source Details from the UDA Database records

    // Very primitive: Need to replicate fully the udaGetAPI arguments from the database

    if (pathway == 1) { // Request via the Database

        std::string signal;
        std::string source;

        signal = fmt::format("{}::{}", data_source->archive, signal_desc->signal_name);
        source = fmt::format("{}::{}", data_source->device_name, data_source->exp_number);

        if (data_source->server[0] != '\0') {
            char *p = nullptr, *s = nullptr;
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
                if (strcasecmp(old_host_.c_str(), data_source->server) != 0) {
                    old_host_ = data_source->server;
                    udaPutServerHost(data_source->server);
                }
                if (IsNumber(&p[1])) {
                    newPort = atoi(&p[1]);
                    if (newPort != old_port_) {
                        udaPutServerPort(newPort);
                        old_port_ = newPort;
                    }
                } else {
                    error(plugin_interface,
                          "The Server Port must be an Integer Number passed using the formats 'server:port' or 'server "
                          "port'");
                }
            } else {
                if (strcasecmp(old_host_.c_str(), data_source->server) != 0) {
                    old_host_ = data_source->server;
                    udaPutServerHost(data_source->server);
                }
            }
        } else {
            error(plugin_interface, "No Server has been specified!");
        }

        debug(plugin_interface, "UDA Server Host for UDA Plugin {}\n", data_source->server);
        debug(plugin_interface, "UDA Server Port for UDA Plugin {}\n", newPort);
        debug(plugin_interface, "Calling udaGetAPI API (Database based Request)\n");
        debug(plugin_interface, "Signal: {}\n", signal.c_str());
        debug(plugin_interface, "Source: {}\n", source.c_str());

        handle = udaGetAPI(signal.c_str(), source.c_str());

    } else if (pathway == 2) {

        char source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Device redirect or server protocol

        strcpy(request->server, request->path); // Extract the Server Name and Port
        char *p = nullptr, *s = nullptr;

        if ((s = strstr(data_source->server, "SSL://")) != nullptr) {
            if ((p = strchr(s + 6, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';           // Break the String (work)
                strcpy(source, p + 1); // Extract the Source URL Argument
            } else {
                error(plugin_interface,
                      "The Remote Server Data Source specified does not comply with the naming model: "
                      "serverHost:port/sourceURL");
            }
        } else {
            if ((p = strchr(request->server, '/')) != nullptr) {
                // Isolate the Server from the source server:port/source
                p[0] = '\0';           // Break the String (work)
                strcpy(source, p + 1); // Extract the Source URL Argument
            } else {
                error(plugin_interface,
                      "The Remote Server Data Source specified does not comply with the naming model: "
                      "serverHost:port/sourceURL");
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
            if (strcasecmp(old_host_.c_str(), request->server) != 0) {
                old_host_ = request->server;
                udaPutServerHost(request->server); // different host name?
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != old_port_) {
                    udaPutServerPort(newPort);
                    old_port_ = newPort;
                }
            } else {
                error(plugin_interface,
                      "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server "
                      "port'");
            }
        } else {
            if (strcasecmp(old_host_.c_str(), request->server) != 0) {
                old_host_ = request->server;
                udaPutServerHost(request->server);
            }
        }

        debug(plugin_interface, "UDA Server Host for UDA Plugin {}\n", request->server);
        debug(plugin_interface, "UDA Server Port for UDA Plugin {}\n", newPort);
        debug(plugin_interface, "Calling udaGetAPI API (Device redirect or server protocol based Request)\n");
        debug(plugin_interface, "Signal: {}\n", request->signal);
        debug(plugin_interface, "Source: {}\n", source);

        handle = udaGetAPI(request->signal, source);

    } else if (pathway == 3) {

        //----------------------------------------------------------------------
        // Function library

        const char* host = nullptr;
        bool isHost = udaPluginFindStringArg(plugin_interface, &host, "host");

        const char* signal = nullptr;
        bool isSignal = udaPluginFindStringArg(plugin_interface, &signal, "signal");

        const char* source = nullptr;
        bool isSource = udaPluginFindStringArg(plugin_interface, &source, "source");

        bool isPort = udaPluginFindIntArg(plugin_interface, &newPort, "port");

        // Set host and port

        if (isHost && strcasecmp(old_host_.c_str(), host) != 0) {
            old_host_ = host;
            udaPutServerHost(host);
        }
        if (isPort && old_port_ != newPort) {
            old_port_ = newPort;
            udaPutServerPort(newPort);
        }

        debug(plugin_interface, "UDA Server Host for UDA Plugin {}\n", host);
        debug(plugin_interface, "UDA Server Port for UDA Plugin {}\n", newPort);
        debug(plugin_interface, "Calling udaGetAPI API (plugin library method based Request)\n");

        if (isSignal && isSource) {
            debug(plugin_interface, "Signal: {}\n", signal);
            debug(plugin_interface, "idamAPIPlugin; Source: {}\n", source);
            handle = udaGetAPI(signal, source);
        } else if (isSignal) {
            debug(plugin_interface, "Signal: {}\n", signal);
            debug(plugin_interface, "idamAPIPlugin; Source: {}\n", request->source);
            handle = udaGetAPI(signal, request->source);
        } else {
            error(plugin_interface, "A data object (signal) has not been specified!");
        }
    } else if (pathway == 4) {

        char source[2 * MAXNAME + 2];

        //----------------------------------------------------------------------
        // Server protocol

        strcpy(source, request->file); // The Source URL Argument

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

        if (p != nullptr) {                                            // look for a port number in the server name
            p[0] = '\0';                                               // Split
            if (strcasecmp(old_host_.c_str(), request->server) != 0) { // Different Hosts?
                old_host_ = request->server;
                udaPutServerHost(request->server); // Change to a different host name
            }
            if (IsNumber(&p[1])) {
                newPort = atoi(&p[1]);
                if (newPort != old_port_) {
                    // Different Ports?
                    udaPutServerPort(newPort);
                    old_port_ = newPort;
                }
            } else {
                error(plugin_interface,
                      "The Server Port must be an Integer Number passed using the format 'server:port'  or 'server "
                      "port'");
            }
        } else {
            // No port number passed
            if (strcasecmp(old_host_.c_str(), request->server) != 0) { // Different Hosts?
                old_host_ = request->server;
                udaPutServerHost(request->server);
            }
        }

        debug(plugin_interface, "UDA Server Host for UDA Plugin {}\n", request->server);
        debug(plugin_interface, "UDA Server Port for UDA Plugin {}\n", newPort);
        debug(plugin_interface, "Calling udaGetAPI API (Server protocol based Request)\n");
        debug(plugin_interface, "Signal: {}\n", request->signal);
        debug(plugin_interface, "Source: {}\n", source);

        handle = udaGetAPI(request->signal, source);
    }

    udaResetPrivateFlag(PRIVATEFLAG_FULLRESET);
    udaResetClientFlag(CLIENTFLAG_FULLRESET);

    //----------------------------------------------------------------------
    // Test for Errors: Close Socket and Free heap

    debug(plugin_interface, "Returned from udaGetAPI API: handle = {}, error code = {}\n", handle,
          udaGetErrorCode(handle));

    if (handle < 0) {
        error(plugin_interface, udaGetServerErrorStackRecordMsg(0));
    } else if ((err = udaGetErrorCode(handle)) != 0) {
        error(plugin_interface, udaGetErrorMsg(handle));
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

    if (udaGetClientVersion() >= 7) {
        // This should contain everything!
        *data_block = *getDataBlock(handle);
    } else { // use abstraction functions

        // Straight structure mapping causes potential problems when the client library uses different versions
        // of the DATA_BLOCK structure
        // or its component structure CLIENT_BLOCK
        // or the initialisation is incomplete!
        // Write the structure components element by element! (Ignore the CLIENT_BLOCK component)

        DATA_BLOCK db;
        initDataBlock(&db);

        auto odb = (OLD_DATA_BLOCK*)getDataBlock(handle);

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

        db.dims = odb->dims; // These have not changed between versions 6 & 7
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

    debug(plugin_interface, "Exit\n");

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
    // Namespace issues: Both the Client and the Server use the same functions to Query. The PRE_LOAD requirement of
    // MDS+ causes the UDA client library to be loaded ahead of the server library: Result confusion and seg fault
    // errors. Need to add unique name component to UDA client server to provide namespace separation. Prepare a reduced
    // set of external symbols for the client library attached to the server!

    // Structured data are best served as serialised objects - no messy heap logs to free
    // Data should be freed with a 'reset' method call after the data have been transmitted
    // Add a new 'freeImmediate' plugin property that causes a heap free without other state changes, e.g. socket

    // Problem tests: 115, 118, 124, 141, 142

    //----------------------------------------------------------------------

    return err;
}
