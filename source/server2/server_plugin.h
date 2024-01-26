#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include "export.h"
#include "plugins.hpp"
#include "server.hpp"
#include "udaPlugin.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

typedef struct UdaPluginInterface {  // Standard Plugin interface
    unsigned short interfaceVersion;  // Interface Version
    unsigned short pluginVersion;     // Plugin Version
    unsigned short sqlConnectionType; // Which SQL is the server connected to
    unsigned short verbose;           // Spare! Use (errout!=NULL) instead  *** Deprecated
    unsigned short housekeeping;      // Housekeeping Directive
    unsigned short changePlugin;      // Use a different Plugin to access the data
    FILE* dbgout;
    FILE* errout;
    DATA_BLOCK* data_block;
    REQUEST_DATA* request_data;
    CLIENT_BLOCK* client_block;
    DATA_SOURCE* data_source;
    SIGNAL_DESC* signal_desc;
    const ENVIRONMENT* environment; // Server environment
    LOGMALLOCLIST* logmalloclist;
    USERDEFINEDTYPELIST* userdefinedtypelist;
    void* sqlConnection;          // Opaque structure
    const PLUGINLIST* pluginList; // List of data readers, filters, models, and servers
    UDA_ERROR_STACK error_stack;
} UDA_PLUGIN_INTERFACE;

namespace uda
{

int serverRedirectStdStreams(int reset);

int serverPlugin(RequestData* request, DataSource* data_source, SignalDesc* signal_desc, const Plugins& plugins,
                 const Environment* environment);

int provenancePlugin(ClientBlock* client_block, RequestData* original_request, const Plugins& plugins,
                     const char* logRecord, const server::Environment& environment, uda::MetadataBlock& metadata);

boost::optional<PluginData> find_metadata_plugin(const Plugins& plugins, const server::Environment& environment);

int call_metadata_plugin(const PluginData& plugin, RequestData* request_block, const server::Environment& environment,
                         const uda::Plugins& plugins, uda::MetadataBlock& metadata);

} // namespace uda

#endif // UDA_SERVER_SERVERPLUGIN_H
