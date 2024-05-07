#pragma once

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "plugins/udaPlugin.h"
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

int server_plugin(const config::Config& config, uda::client_server::RequestData *request, uda::client_server::DataSource *data_source,
                  uda::client_server::SignalDesc *signal_desc, const Plugins& plugins);

int provenance_plugin(const config::Config& config, uda::client_server::ClientBlock *client_block, uda::client_server::RequestData *original_request,
                      const Plugins& plugins, const char* logRecord, MetadataBlock& metadata);

int call_metadata_plugin(const config::Config& config, const uda::plugins::PluginData& plugin,
                         uda::client_server::RequestData* request_block, const Plugins& plugins, MetadataBlock& metadata);

boost::optional<uda::plugins::PluginData> find_metadata_plugin(const config::Config& config, const Plugins& plugins);

} // namespace uda::server
