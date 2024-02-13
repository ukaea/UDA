#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "plugins/udaPlugin.h"
#include "uda/types.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

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

int call_metadata_plugin(const uda::plugins::PluginData& plugin, uda::client_server::RequestData* request_block,
                         const server::Environment& environment, const uda::Plugins& plugins,
                         uda::MetadataBlock& metadata);

boost::optional<uda::plugins::PluginData> find_metadata_plugin(const Plugins& plugins,
                                                               const server::Environment& environment);

} // namespace uda

#endif // UDA_SERVER_SERVERPLUGIN_H
