#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>
#include "plugins.hpp"
#include "server.hpp"

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

namespace uda {

int serverRedirectStdStreams(int reset);

int serverPlugin(RequestData* request, DataSource* data_source, SignalDesc* signal_desc,
                 const Plugins& plugins, const Environment* environment);

int provenancePlugin(ClientBlock* client_block, RequestData* original_request, const Plugins& plugins,
                     const char* logRecord, const server::Environment& environment, uda::MetadataBlock& metadata);

boost::optional<PluginData> find_metadata_plugin(const Plugins& plugins, const server::Environment& environment);

int call_metadata_plugin(const PluginData& plugin, RequestData* request_block, const server::Environment& environment,
                         const uda::Plugins& plugins, uda::MetadataBlock& metadata);

}

#endif // UDA_SERVER_SERVERPLUGIN_H
