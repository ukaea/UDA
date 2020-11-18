#ifndef UDA_SERVER_SERVERPLUGIN_H
#define UDA_SERVER_SERVERPLUGIN_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void allocPluginList(int count, PLUGINLIST* plugin_list);
LIBRARY_API void freePluginList(PLUGINLIST* plugin_list);
LIBRARY_API void initPluginData(PLUGIN_DATA* plugin);
LIBRARY_API void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment);
LIBRARY_API int udaServerRedirectStdStreams(int reset);
LIBRARY_API int udaServerPlugin(REQUEST_BLOCK* request_block, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc,
                                const PLUGINLIST* plugin_list, const ENVIRONMENT* environment);
LIBRARY_API int udaProvenancePlugin(CLIENT_BLOCK* client_block, REQUEST_BLOCK* original_request_block,
                                    DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, const PLUGINLIST* plugin_list,
                                    char* logRecord, const ENVIRONMENT* environment);
LIBRARY_API int udaServerMetaDataPluginId(const PLUGINLIST* plugin_list, const ENVIRONMENT* environment);
LIBRARY_API int udaServerMetaDataPlugin(const PLUGINLIST* plugin_list, int plugin_id, REQUEST_BLOCK* request_block,
                                        SIGNAL_DESC* signal_desc, SIGNAL* signal_rec, DATA_SOURCE* data_source,
                                        const ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERPLUGIN_H
