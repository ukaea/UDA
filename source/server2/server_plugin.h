#pragma once

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "plugins/udaPlugin.h"
#include "uda/types.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

namespace uda::server
{

class Plugins;
struct MetadataBlock;
class Environment;
class Config;

int server_redirect_std_streams(const Config& config, int reset);

int server_plugin(uda::client_server::RequestData *request, uda::client_server::DataSource *data_source,
                  uda::client_server::SignalDesc *signal_desc, const Plugins& plugins,
                  const uda::client_server::Environment* environment);

int provenance_plugin(const Config& config, uda::client_server::ClientBlock *client_block, uda::client_server::RequestData *original_request,
                      const Plugins& plugins, const char* logRecord, const server::Environment& environment,
                      MetadataBlock& metadata);

int call_metadata_plugin(const Config& config, const uda::plugins::PluginData& plugin, uda::client_server::RequestData* request_block,
                         const server::Environment& environment, const Plugins& plugins, MetadataBlock& metadata);

boost::optional<uda::plugins::PluginData> find_metadata_plugin(const Config& config, const Plugins& plugins,
                                                               const server::Environment& environment);

} // namespace uda::server
