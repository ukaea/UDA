#pragma once

#include "clientserver/uda_structs.h"
#include "plugins.hpp"

namespace uda::config {
class Config;
}

namespace uda::server
{

class Plugins;
class Environment;

int server_redirect_std_streams(const config::Config& config, int reset);

int server_plugin(const config::Config& config, client_server::RequestData *request, client_server::MetaData *meta_data,
                  const Plugins& plugins);

int provenance_plugin(const config::Config& config, client_server::ClientBlock *client_block, client_server::RequestData *original_request,
                      const Plugins& plugins, const char* logRecord, client_server::MetaData& metadata);

int call_metadata_plugin(const config::Config& config, const client_server::PluginData& plugin,
                         client_server::RequestData* request_block, const Plugins& plugins, client_server::MetaData& metadata);

boost::optional<const client_server::PluginData&> find_metadata_plugin(const config::Config& config, const Plugins& plugins);

} // namespace uda::server
