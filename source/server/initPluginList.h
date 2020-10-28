#ifndef UDA_SERVER_INITPLUGINLIST_H
#define UDA_SERVER_INITPLUGINLIST_H

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_INITPLUGINLIST_H
