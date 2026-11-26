// Stand-alone host plugin interfaces (server only: uses the server environment and plugin list)

#include "pluginInterface.h"

#include <cstdlib>

#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>

#include "getServerEnvironment.h"
#include "initPluginList.h"
#include "serverPlugin.h"

IDAM_PLUGIN_INTERFACE* udaCreatePluginInterface(const char* request)
{
    auto plugin_interface = (IDAM_PLUGIN_INTERFACE*)calloc(1, sizeof(IDAM_PLUGIN_INTERFACE));
    udaInitHostPluginInterface(plugin_interface, nullptr);

    auto environment = getServerEnvironment();
    auto plugin_list = (PLUGINLIST*)calloc(1, sizeof(PLUGINLIST));

    initPluginList(plugin_list, environment);

    auto request_data = (REQUEST_DATA*)calloc(1, sizeof(REQUEST_DATA));
    initRequestData(request_data);
    if (request != nullptr) {
        copyString(request, request_data->signal, MAXMETA);
    }
    makeRequestData(request_data, *plugin_list, environment);

    auto user_defined_type_list = (USERDEFINEDTYPELIST*)calloc(1, sizeof(USERDEFINEDTYPELIST));
    auto log_malloc_list = (LOGMALLOCLIST*)calloc(1, sizeof(LOGMALLOCLIST));

    plugin_interface->request_data = request_data;
    plugin_interface->pluginList = plugin_list;
    plugin_interface->environment = environment;
    plugin_interface->userdefinedtypelist = user_defined_type_list;
    plugin_interface->logmalloclist = log_malloc_list;

    return plugin_interface;
}

void udaFreePluginInterface(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    if (plugin_interface == nullptr) {
        return;
    }
    free(plugin_interface->request_data);
    freeUserDefinedTypeList(plugin_interface->userdefinedtypelist);
    free(plugin_interface->userdefinedtypelist);
    freeMallocLogList(plugin_interface->logmalloclist);
    free(plugin_interface->logmalloclist);
    freePluginList((PLUGINLIST*)plugin_interface->pluginList);
    free((PLUGINLIST*)plugin_interface->pluginList);
    // The environment is the server's static environment (getServerEnvironment()) and must not be freed
    free(plugin_interface);
}
