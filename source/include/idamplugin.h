// Manage the List of available data reader plugins

#ifndef IdamPluginInclude
#define IdamPluginInclude

#include "idamclientserver.h"
#include "idamgenstructpublic.h"

#include <clientserver/idamStructs.h>
#include <logging/idamLog.h>
#include <clientserver/idamErrorLog.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXFUNCTIONNAME     256

// plugin State

#define PLUGINNOTEXTERNAL       0
#define PLUGINEXTERNAL          1           // The plugin resides in an external shared library
#define PLUGINNOTOPERATIONAL    0
#define PLUGINOPERATIONAL       1

// privacy

#define PLUGINPRIVATE       1           // Only internal users can use the service (access the data!)
#define PLUGINPUBLIC        0           // All users - internal and external - can use the service

// List of available data reader plugins

#define IDAM_NETRC          ".netrc"
#define IDAM_PROXYHOST      "proxypac"
#define IDAM_PROXYPORT      "8080"
#define IDAM_PROXYPAC       "fproxy.pac"
#define IDAM_PROXYUSER      "nobody"
#define IDAM_PROXYPROTOCOL  "http"

// SQL Connection Types

#define PLUGINSQLNOTKNOWN   	0
#define PLUGINSQLPOSTGRES   	1
#define PLUGINSQLMYSQL      	2
#define PLUGINSQLMONGODB	3

extern unsigned short pluginClass;

enum pluginClass {
    PLUGINUNKNOWN,
    PLUGINFILE,         // File format access
    PLUGINSERVER,       // Server protocol access
    PLUGINFUNCTION,     // Server-side function transformation
    PLUGINDEVICE,       // Server to Server chaining, i.e. Pass the request to an external server
    PLUGINOTHER
};

typedef void (* ADDIDAMERRORFUNP)(IDAMERRORSTACK*, int, char*, int, char*);   // Write to the Error Log

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
    ENVIRONMENT* environment;           // Server environment
    void* sqlConnection;                // Opaque structure
    PLUGINLIST* pluginList;             // List of data readers, filters, models, and servers
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
    unsigned short class;               // the plugin class: File, Server, Function, Device
    unsigned short external;            // Flag the plugin is accessed via a separate shared library
    unsigned short status;              // Plugin operational: external library opened or internal
    unsigned short private;             // The service is private and can NOT be used by external clients
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

// Prototypes

extern PLUGINLIST pluginList;

unsigned short findStringValue(NAMEVALUELIST* namevaluelist, char** value, const char* name);

unsigned short findValue(NAMEVALUELIST* namevaluelist, const char* name);

unsigned short findIntValue(NAMEVALUELIST* namevaluelist, int* value, const char* name);

unsigned short findShortValue(NAMEVALUELIST* namevaluelist, short* value, const char* name);

unsigned short findFloatValue(NAMEVALUELIST* namevaluelist, float* values, const char* name);

unsigned short findIntArray(NAMEVALUELIST* namevaluelist, int** values, size_t* nvalues, const char* name);

unsigned short findFloatArray(NAMEVALUELIST* namevaluelist, float** values, size_t* nvalues, const char* name);

int callPlugin(PLUGINLIST* pluginlist, const char* request, const IDAM_PLUGIN_INTERFACE* old_plugin_interface);

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)
#define UNIQUE_VAR(NAME) __func__##NAME##__

#define RAISE_PLUGIN_ERROR(MSG) \
{ int UNIQUE_VAR(err) = 999; \
IDAM_LOGF(LOG_ERROR, "%s\n", MSG); \
addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, UNIQUE_VAR(err), MSG); \
return UNIQUE_VAR(err); }

#define RAISE_PLUGIN_ERROR_EX(MSG, CODE) \
int UNIQUE_VAR(err) = 999; \
IDAM_LOGF(LOG_ERROR, "%s", MSG); \
addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, UNIQUE_VAR(err), MSG); \
CODE; \
return UNIQUE_VAR(err);

#define FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, TYPE) \
if (!find##TYPE##Value(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))) { \
    RAISE_PLUGIN_ERROR("Required argument '" QUOTE(VARIABLE) "' not given"); \
}

#define FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, TYPE) \
if (!find##TYPE##Array(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))) { \
    RAISE_PLUGIN_ERROR("Required argument '" QUOTE(VARIABLE) "' not given"); \
}

#define FIND_REQUIRED_INT_VALUE(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Int)
#define FIND_REQUIRED_FLOAT_VALUE(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Float)
#define FIND_REQUIRED_STRING_VALUE(NAME_VALUE_LIST, VARIABLE)   FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, String)

#define FIND_REQUIRED_INT_ARRAY(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, Int)
#define FIND_REQUIRED_FLOAT_ARRAY(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, Float)

#define FIND_INT_VALUE(NAME_VALUE_LIST, VARIABLE)       findIntValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_SHORT_VALUE(NAME_VALUE_LIST, VARIABLE)     findShortValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_FLOAT_VALUE(NAME_VALUE_LIST, VARIABLE)     findFloatValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_STRING_VALUE(NAME_VALUE_LIST, VARIABLE)    findStringValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))

#define FIND_INT_ARRAY(NAME_VALUE_LIST, VARIABLE)       findIntArray(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define FIND_FLOAT_ARRAY(NAME_VALUE_LIST, VARIABLE)     findFloatArray(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))

#define CALL_PLUGIN(PLUGIN_INTERFACE, FMT, ...) \
{ char UNIQUE_VAR(request)[1024]; \
snprintf(UNIQUE_VAR(request), FMT, __VA_ARGS__); \
UNIQUE_VAR(request)[1023] = '\0'; \
int UNIQUE_VAR(err) = callPlugin(PLUGIN_INTERFACE->pluginList, UNIQUE_VAR(request), PLUGIN_INTERFACE); \
if (UNIQUE_VAR(err)) { \
    RAISE_PLUGIN_ERROR("Plugin call failed"); \
} }

#ifdef __cplusplus
}
#endif

#endif
