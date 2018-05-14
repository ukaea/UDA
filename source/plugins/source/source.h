#ifndef IDAM_PLUGINS_SOURCE_SOURCE_H
#define IDAM_PLUGINS_SOURCE_SOURCE_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <plugins/udaPlugin.h>
#include <plugins/serverPlugin.h>
#include <client/udaClient.h>
#include <clientserver/compressDim.h>

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

int source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

//--------------------------------------------------------------------------- 
// Prototypes 


#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_SOURCE_SOURCE_H
