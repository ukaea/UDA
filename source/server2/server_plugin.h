#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "uda/types.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

typedef struct PluginList PLUGINLIST;

typedef struct UdaPluginInterface {   // Standard Plugin interface
    unsigned short interfaceVersion;  // Interface Version
    unsigned short pluginVersion;     // Plugin Version
    unsigned short sqlConnectionType; // Which SQL is the server connected to
    unsigned short verbose;           // Spare! Use (errout!=NULL) instead  *** Deprecated
    unsigned short housekeeping;      // Housekeeping Directive
    unsigned short changePlugin;      // Use a different Plugin to access the data
    FILE* dbgout;
    FILE* errout;
    uda::client_server::DATA_BLOCK* data_block;
    uda::client_server::REQUEST_DATA* request_data;
    uda::client_server::CLIENT_BLOCK* client_block;
    uda::client_server::DATA_SOURCE* data_source;
    uda::client_server::SIGNAL_DESC* signal_desc;
    const uda::client_server::ENVIRONMENT* environment; // Server environment
    LOGMALLOCLIST* logmalloclist;
    USERDEFINEDTYPELIST* userdefinedtypelist;
    void* sqlConnection;          // Opaque structure
    const PLUGINLIST* pluginList; // List of data readers, filters, models, and servers
    uda::client_server::UDA_ERROR_STACK error_stack;
} UDA_PLUGIN_INTERFACE;

struct PluginData;

namespace uda
{

class Plugins;
struct MetadataBlock;

namespace server
{
class Environment;
}

int serverRedirectStdStreams(int reset);

int serverPlugin(uda::client_server::RequestData* request, uda::client_server::DataSource* data_source,
                 uda::client_server::SignalDesc* signal_desc, const Plugins& plugins,
                 const uda::client_server::Environment* environment);

int provenancePlugin(uda::client_server::ClientBlock* client_block, uda::client_server::RequestData* original_request,
                     const Plugins& plugins, const char* logRecord, const server::Environment& environment,
                     uda::MetadataBlock& metadata);

int call_metadata_plugin(const PluginData& plugin, uda::client_server::RequestData* request_block,
                         const server::Environment& environment, const uda::Plugins& plugins,
                         uda::MetadataBlock& metadata);

boost::optional<PluginData> find_metadata_plugin(const Plugins& plugins, const server::Environment& environment);

} // namespace uda

#endif // UDA_SERVER_SERVERPLUGIN_H
