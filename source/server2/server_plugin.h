#pragma once

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "uda/types.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

namespace uda::config {
class Config;
}

namespace uda::server
{

class Plugins;
struct MetadataBlock;
class Environment;

int server_redirect_std_streams(const config::Config& config, int reset);

int server_plugin(const config::Config& config, client_server::RequestData *request, client_server::DataSource *data_source,
                  client_server::SignalDesc *signal_desc, const Plugins& plugins);

int provenance_plugin(const config::Config& config, client_server::ClientBlock *client_block, client_server::RequestData *original_request,
                      const Plugins& plugins, const char* logRecord, MetadataBlock& metadata);

int call_metadata_plugin(const config::Config& config, const client_server::PluginData& plugin,
                         client_server::RequestData* request_block, const Plugins& plugins, MetadataBlock& metadata);

boost::optional<const client_server::PluginData&> find_metadata_plugin(const config::Config& config, const Plugins& plugins);

} // namespace uda::server
