#ifndef UDA_PLUGINS_PLUGINUTILS_H
#define UDA_PLUGINS_PLUGINUTILS_H

#include <plugins/udaPlugin.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

#ifdef __cplusplus
extern "C" {
#endif

void allocPluginList(int count, PLUGINLIST* plugin_list);

void closePluginList(const PLUGINLIST* plugin_list);

void freePluginList(PLUGINLIST* plugin_list);

void initPluginData(PLUGIN_DATA* plugin);

void printPluginList(FILE* fd, const PLUGINLIST* plugin_list);

int findPluginIdByRequest(int request, const PLUGINLIST* plugin_list);

int findPluginIdByFormat(const char* format, const PLUGINLIST* plugin_list);

int findPluginIdByDevice(const char* device, const PLUGINLIST* plugin_list);

int findPluginRequestByFormat(const char* format, const PLUGINLIST* plugin_list);

int findPluginRequestByExtension(const char* extension, const PLUGINLIST* plugin_list);

void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment);

int idamServerRedirectStdStreams(int reset);

int callPlugin(const PLUGINLIST* pluginlist, const char* request, const IDAM_PLUGIN_INTERFACE* old_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_PLUGINUTILS_H
