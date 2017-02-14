#ifndef IDAM_SERVER_IDAMSERVERPLUGIN_H
#define IDAM_SERVER_IDAMSERVERPLUGIN_H

#include <plugins/idamPlugin.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

void allocPluginList(int count, PLUGINLIST * plugin_list);

void closePluginList(PLUGINLIST * plugin_list);

void freePluginList(PLUGINLIST * plugin_list);

void initPlugin(PLUGIN_DATA * plugin);

void printPluginList(FILE * fd, PLUGINLIST * plugin_list);

int findPluginIdByRequest(int request, PLUGINLIST * plugin_list);

int findPluginIdByFormat(const char * format, PLUGINLIST * plugin_list);

int findPluginIdByDevice(const char * device, PLUGINLIST * plugin_list);

int findPluginRequestByFormat(const char * format, PLUGINLIST * plugin_list);

int findPluginRequestByExtension(const char * extension, PLUGINLIST * plugin_list);

void initPluginList(PLUGINLIST * plugin_list);

int idamServerRedirectStdStreams(int reset);

// 1. open configuration file
// 2. read plugin details
//   2.1 format
//   2.2 file or server
//   2.3 library name
//   2.4 symbol name
// 3. check format is unique
// 4. issue a request ID
// 5. open the library
// 6. get plugin function address
// 7. close the file

int idamServerPlugin(REQUEST_BLOCK * request_block, DATA_SOURCE * data_source, SIGNAL_DESC * signal_desc, PLUGINLIST * plugin_list);

//------------------------------------------------------------------------------------------------
// Provenance gathering plugin with a separate database.
// Functionality exposed to both server (special plugin with standard methods)
// and client application (behaves as a normal plugin)
//
// Server needs are (private to the server):
//	record (put) the original and the actual signal and source terms with the source file DOI
//	record (put) the server log record
// Client needs are (the plugin exposes these to the client in the regular manner):
//	list all provenance records for a specific client DOI - must be given
//	change provenance records status to closed
//	delete all closed records for a specific client DOI
//
// changePlugin option disabled in this context
// private malloc log and userdefinedtypelist

int idamProvenancePlugin(CLIENT_BLOCK * client_block, REQUEST_BLOCK * original_request_block,
                         DATA_SOURCE * data_source, SIGNAL_DESC * signal_desc, PLUGINLIST * plugin_list,
                         char * logRecord);

int idamServerMetaDataPluginId(PLUGINLIST* plugin_list);

int idamServerMetaDataPlugin(PLUGINLIST * plugin_list, int plugin_id, REQUEST_BLOCK * request_block,
                             SIGNAL_DESC * signal_desc, DATA_SOURCE * data_source);

#endif // IDAM_SERVER_IDAMSERVERPLUGIN_H

