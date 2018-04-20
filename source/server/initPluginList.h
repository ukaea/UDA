#ifndef UDA_SERVER_INITPLUGINLIST_H
#define UDA_SERVER_INITPLUGINLIST_H

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_INITPLUGINLIST_H
