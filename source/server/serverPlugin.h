#pragma once

#include <cstdio>
#include <uda/types.h>

#include "clientserver/udaStructs.h"
#include "plugins/udaPlugin.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

namespace uda::server
{

int findPluginIdByRequest(int request, const uda::plugins::PluginList* plugin_list);

int findPluginIdByFormat(const char* format, const uda::plugins::PluginList* plugin_list);

int findPluginIdByDevice(const char* device, const uda::plugins::PluginList* plugin_list);

int findPluginRequestByFormat(const char* format, const uda::plugins::PluginList* plugin_list);

int findPluginRequestByExtension(const char* extension, const uda::plugins::PluginList* plugin_list);

void allocPluginList(int count, uda::plugins::PluginList* plugin_list);

void freePluginList(uda::plugins::PluginList* plugin_list);

void initPluginData(uda::plugins::PluginData* plugin);

int udaServerRedirectStdStreams(int reset);

int udaServerPlugin(uda::client_server::RequestData* request, uda::client_server::DataSource* data_source,
                    uda::client_server::SignalDesc* signal_desc, const uda::plugins::PluginList* plugin_list,
                    const uda::client_server::Environment* environment);

int udaProvenancePlugin(uda::client_server::ClientBlock* client_block,
                        uda::client_server::RequestData* original_request,
                        uda::client_server::DataSource* data_source, uda::client_server::SignalDesc* signal_desc,
                        const uda::plugins::PluginList* plugin_list, const char* logRecord,
                        const uda::client_server::Environment* environment);

int udaServerMetaDataPluginId(const uda::plugins::PluginList* plugin_list,
                              const uda::client_server::Environment* environment);

int udaServerMetaDataPlugin(const uda::plugins::PluginList* plugin_list, int plugin_id,
                            uda::client_server::RequestData* request_block,
                            uda::client_server::SignalDesc* signal_desc, uda::client_server::Signal* signal_rec,
                            uda::client_server::DataSource* data_source,
                            const uda::client_server::Environment* environment);

} // namespace uda::server
