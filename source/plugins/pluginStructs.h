#ifndef UDA_SERVER_IDAMPLUGINSTRUCTS_H
#define UDA_SERVER_IDAMPLUGINSTRUCTS_H

#include <stdio.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PluginList;              // Forward declaration
typedef struct PluginList PLUGINLIST;

typedef struct IdamPluginInterface {    // Standard Plugin interface
    unsigned short interfaceVersion;    // Interface Version
    unsigned short pluginVersion;       // Plugin Version
    unsigned short sqlConnectionType;   // Which SQL is the server connected to
    unsigned short verbose;             // Spare! Use (errout!=NULL) instead  *** Deprecated
    unsigned short housekeeping;        // Housekeeping Directive
    unsigned short changePlugin;        // Use a different Plugin to access the data
    FILE* dbgout;
    FILE* errout;
    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;
    CLIENT_BLOCK* client_block;
    DATA_SOURCE* data_source;
    SIGNAL_DESC* signal_desc;
    const ENVIRONMENT* environment;           // Server environment
    LOGMALLOCLIST* logmalloclist;
    USERDEFINEDTYPELIST* userdefinedtypelist;
    void* sqlConnection;                // Opaque structure
    const PLUGINLIST* pluginList;       // List of data readers, filters, models, and servers
} IDAM_PLUGIN_INTERFACE;

typedef int (* PLUGINFUNP)(IDAM_PLUGIN_INTERFACE*);             // Plugin function type

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
    unsigned short interfaceVersion;    // Maximum interface version the plugin is compliant with (Minimum is 1)
    void* pluginHandle;                 // Plugin Library handle
    PLUGINFUNP idamPlugin;              // Plugin function address
} PLUGIN_DATA;

struct PluginList {
    int count;              // the number of plugins
    int mcount;             // malloc count allocated
    PLUGIN_DATA* plugin;
};

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_IDAMPLUGINSTRUCTS_H
