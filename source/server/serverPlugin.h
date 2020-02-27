#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include <plugins/udaPlugin.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

void allocPluginList(int count, PLUGINLIST* plugin_list);

void freePluginList(PLUGINLIST* plugin_list);

void initPluginData(PLUGIN_DATA* plugin);

void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment);

int idamServerRedirectStdStreams(int reset);

int idamServerPlugin(REQUEST_BLOCK* request_block, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc,
                     const PLUGINLIST* plugin_list, const ENVIRONMENT* environment);

int idamProvenancePlugin(CLIENT_BLOCK* client_block, REQUEST_BLOCK* original_request_block,
                         DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, const PLUGINLIST* plugin_list,
                         char* logRecord, const ENVIRONMENT* environment);

int idamServerMetaDataPluginId(const PLUGINLIST* plugin_list, const ENVIRONMENT* environment);

int idamServerMetaDataPlugin(const PLUGINLIST* plugin_list, int plugin_id, REQUEST_BLOCK* request_block,
                             SIGNAL_DESC* signal_desc, SIGNAL* signal_rec, DATA_SOURCE* data_source,
                             const ENVIRONMENT* environment);

#endif // UDA_SERVER_SERVERPLUGIN_H
