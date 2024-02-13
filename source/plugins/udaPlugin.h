#pragma once

#include "clientserver/udaStructs.h"
#include <cstdio>

typedef struct CUdaPluginInterface {
} UDA_PLUGIN_INTERFACE;

namespace uda::plugins
{

enum pluginClass {
    UDA_PLUGIN_CLASS_UNKNOWN,
    UDA_PLUGIN_CLASS_FILE,     // File format access
    UDA_PLUGIN_CLASS_SERVER,   // Server protocol access
    UDA_PLUGIN_CLASS_FUNCTION, // Server-side function transformation
    UDA_PLUGIN_CLASS_DEVICE,   // Server to Server chaining, i.e. Pass the request to an external server
    UDA_PLUGIN_CLASS_OTHER
};

typedef int (*PLUGINFUNP)(UDA_PLUGIN_INTERFACE*); // Plugin function type

typedef struct PluginData {
    char format[STRING_LENGTH];         // File format, or Function library or Server protocol or External Device name
    char library[STRING_LENGTH];        // external plugin shared library name (must be on Server library search path)
    char symbol[STRING_LENGTH];         // external plugin symbol name
    char method[STRING_LENGTH];         // Method to use for Data Readers (FILE Plugin Class)
    char extension[STRING_LENGTH];      // File Extension (Not Case sensitive)
    char deviceProtocol[STRING_LENGTH]; // Server protocol substitute for Device name
    char deviceHost[STRING_LENGTH];     // Server Host substitute for Device name
    char devicePort[STRING_LENGTH];     // Server Port substitute for Device name
    char desc[STRING_LENGTH];           // Description of the plugin
    char example[STRING_LENGTH];        // Examples of Use
    int request;                        // unique request ID
    unsigned short plugin_class;        // the plugin class: File, Server, Function, Device
    unsigned short external;            // Flag the plugin is accessed via a separate shared library
    unsigned short status;              // Plugin operational: external library opened or internal
    unsigned short is_private;          // The service is private and can NOT be used by external clients
    unsigned short cachePermission;     // The server's internal state may be dependent on previous calls
    // so the returned data are not suitable for caching on the client.
    // This is used to inform the client how to manage the returned data
    unsigned short interfaceVersion; // Maximum interface version the plugin is compliant with (Minimum is 1)
    void* pluginHandle;              // Plugin Library handle
    PLUGINFUNP idamPlugin;           // Plugin function address
} PLUGIN_DATA;

struct PluginList {
    int count;  // the number of plugins
    int mcount; // malloc count allocated
    PLUGIN_DATA* plugin;
};

struct UdaPluginInterface : UDA_PLUGIN_INTERFACE { // Standard Plugin interface
    unsigned short interfaceVersion;               // Interface Version
    unsigned short pluginVersion;                  // Plugin Version
    unsigned short sqlConnectionType;              // Which SQL is the server connected to
    unsigned short verbose;                        // Spare! Use (errout!=NULL) instead  *** Deprecated
    unsigned short housekeeping;                   // Housekeeping Directive
    unsigned short changePlugin;                   // Use a different Plugin to access the data
    FILE* dbgout;
    FILE* errout;
    uda::client_server::DataBlock* data_block;
    uda::client_server::RequestData* request_data;
    uda::client_server::ClientBlock* client_block;
    uda::client_server::DataSource* data_source;
    uda::client_server::SignalDesc* signal_desc;
    const uda::client_server::Environment* environment; // Server environment
    LOGMALLOCLIST* logmalloclist;
    USERDEFINEDTYPELIST* userdefinedtypelist;
    void* sqlConnection;          // Opaque structure
    const PluginList* pluginList; // List of data readers, filters, models, and servers
    uda::client_server::ErrorStack error_stack;
};

} // namespace uda::plugins