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

int udaServerPlugin(uda::client_server::REQUEST_DATA* request, uda::client_server::DATA_SOURCE* data_source,
                    uda::client_server::SIGNAL_DESC* signal_desc, const uda::plugins::PluginList* plugin_list,
                    const uda::client_server::ENVIRONMENT* environment);

int udaProvenancePlugin(uda::client_server::CLIENT_BLOCK* client_block,
                        uda::client_server::REQUEST_DATA* original_request,
                        uda::client_server::DATA_SOURCE* data_source, uda::client_server::SIGNAL_DESC* signal_desc,
                        const uda::plugins::PluginList* plugin_list, const char* logRecord,
                        const uda::client_server::ENVIRONMENT* environment);

int udaServerMetaDataPluginId(const uda::plugins::PluginList* plugin_list,
                              const uda::client_server::ENVIRONMENT* environment);

int udaServerMetaDataPlugin(const uda::plugins::PluginList* plugin_list, int plugin_id,
                            uda::client_server::REQUEST_DATA* request_block,
                            uda::client_server::SIGNAL_DESC* signal_desc, uda::client_server::SIGNAL* signal_rec,
                            uda::client_server::DATA_SOURCE* data_source,
                            const uda::client_server::ENVIRONMENT* environment);

} // namespace uda::server
