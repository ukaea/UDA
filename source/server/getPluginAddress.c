#include "getPluginAddress.h"

#include <dlfcn.h>
#include <stdlib.h>

/**
 * Return the function address for plugin data readers located in external shared libraries
 *
 * @param pluginHandle
 * @param library the full file path name to the registered plugin shared library
 * @param symbol the name of the library api function to be called
 * @param idamPlugin the address of the library function
 * @return
 */
int getPluginAddress(PLUGIN_DATA* plugin_data)
{
    int err = 0;
    int (* fptr)(IDAM_PLUGIN_INTERFACE*);               // Pointer to a Plugin function with standard interface

    plugin_data->idamPlugin = (PLUGINFUNP)NULL;                     // Default

    if (plugin_data->library[0] == '\0' || plugin_data->symbol[0] == '\0') {
        // Nothing to 'point' to! Is this an Error?
        strncpy(plugin_data->loadErrors, "library or symbol not defined", STRING_LENGTH);
        return err;
    }

    const char* plugin_dir = getenv("UDA_PLUGIN_DIR");
    char* full_path;
    if (plugin_dir != NULL) {
        full_path = malloc(strlen(plugin_dir) + strlen(plugin_data->library) + 2);
        sprintf(full_path, "%s/%s", plugin_dir, plugin_data->library);
    } else {
        full_path = strdup(plugin_data->library);
    }

// Open the named library

    if (plugin_data->pluginHandle == NULL) {
        if ((plugin_data->pluginHandle = dlopen(full_path, RTLD_LOCAL | RTLD_NOW)) == NULL) {
            err = 999;
            char* errstr = dlerror();
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Cannot open the target shared library");
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, errstr);
            strncpy(plugin_data->loadErrors, errstr, STRING_LENGTH);
            plugin_data->loadErrors[STRING_LENGTH - 1] = '\0';
            return err;
        }
    }

    free(full_path);

// Register the handle with the plugin manager: Close at server shut down only

// Find the address of the required plugin function

    *(void**)(&fptr) = dlsym(plugin_data->pluginHandle, plugin_data->symbol);

    char* errstr = dlerror();

    if (errstr == NULL) {
        plugin_data->idamPlugin = (PLUGINFUNP)fptr;
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Cannot locate the data reader within the target shared library");
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, errstr);
        dlclose(plugin_data->pluginHandle);
        plugin_data->pluginHandle = NULL;
        strncpy(plugin_data->loadErrors, errstr, STRING_LENGTH);
        plugin_data->loadErrors[STRING_LENGTH - 1] = '\0';
        return err;
    }

    return err;
}
