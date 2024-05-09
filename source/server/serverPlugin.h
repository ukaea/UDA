#pragma once

#include <cstdio>
#include <vector>
#include <uda/types.h>

#include "clientserver/udaStructs.h"
#include "clientserver/plugins.h"
#include "plugins/udaPlugin.h"
#include "structures/genStructs.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

namespace uda::config
{
class Config;
}

typedef struct CUdaPluginInterface {
} UDA_PLUGIN_INTERFACE;

namespace uda::server
{

typedef int (*PLUGINFUNP)(UDA_PLUGIN_INTERFACE*); // Plugin function type

struct UdaPluginInterface : UDA_PLUGIN_INTERFACE { // Standard Plugin interface
    unsigned short interfaceVersion;               // Interface Version
    unsigned short pluginVersion;                  // Plugin Version
    unsigned short sqlConnectionType;              // Which SQL is the server connected to
    unsigned short verbose;                        // Spare! Use (errout!=NULL) instead  *** Deprecated
    unsigned short housekeeping;                   // Housekeeping Directive
    unsigned short changePlugin;                   // Use a different Plugin to access the data
    FILE* dbgout;
    FILE* errout;
    uda::client_server::DataBlock* data_block;
    uda::client_server::RequestData* request_data;
    uda::client_server::ClientBlock* client_block;
    uda::client_server::DataSource* data_source;
    uda::client_server::SignalDesc* signal_desc;
    uda::structures::LogMallocList* logmalloclist;
    uda::structures::UserDefinedTypeList* userdefinedtypelist;
    void* sqlConnection;          // Opaque structure
    const std::vector<client_server::PluginData>* pluginList; // List of data readers, filters, models, and servers
    uda::client_server::ErrorStack error_stack;
    const uda::config::Config* config;
};

int findPluginIdByFormat(const char* format, const std::vector<client_server::PluginData>& plugin_list);

int findPluginIdByDevice(const char* device, const std::vector<client_server::PluginData>& plugin_list);

void freePluginList(std::vector<client_server::PluginData>& plugin_list);

void initPluginData(client_server::PluginData* plugin);

int udaServerRedirectStdStreams(int reset);

int udaServerPlugin(const config::Config& config, client_server::RequestData* request,
                    client_server::DataSource* data_source,
                    client_server::SignalDesc* signal_desc, const std::vector<client_server::PluginData>& plugin_list);

int udaProvenancePlugin(const config::Config& config, client_server::ClientBlock* client_block,
                        client_server::RequestData* original_request, client_server::DataSource* data_source,
                        client_server::SignalDesc* signal_desc, const std::vector<client_server::PluginData>& plugin_list,
                        const char* logRecord);

int udaServerMetaDataPluginId(const config::Config& config, const std::vector<client_server::PluginData>& plugin_list);

int udaServerMetaDataPlugin(const config::Config& config, const std::vector<client_server::PluginData>& plugin_list, int plugin_id,
                            client_server::RequestData* request_block, client_server::SignalDesc* signal_desc,
                            client_server::Signal* signal_rec, client_server::DataSource* data_source);

} // namespace server
