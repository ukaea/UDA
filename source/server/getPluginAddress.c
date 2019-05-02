#include "getPluginAddress.h"

#include <dlfcn.h>
#include <stdlib.h>

#include <clientserver/udaErrors.h>
#include <server/udaServer.h>

/**
 * Return the function address for plugin data readers located in external shared libraries
 *
 * @param pluginHandle
 * @param library the full file path name to the registered plugin shared library
 * @param symbol the name of the library api function to be called
 * @param idamPlugin the address of the library function
 * @return
 */
int getPluginAddress(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* idamPlugin)
{
    int err = 0;
    int (* fptr)(IDAM_PLUGIN_INTERFACE*);               // Pointer to a Plugin function with standard interface

    *idamPlugin = (PLUGINFUNP)NULL;                     // Default

    if (library[0] == '\0' || symbol[0] == '\0') {      // Nothing to 'point' to! Is this an Error?
        return err;
    }

    const char* plugin_dir = getenv("UDA_PLUGIN_DIR");
    char* full_path;
    if (plugin_dir != NULL) {
        full_path = (char*)malloc(strlen(plugin_dir) + strlen(library) + 2);
        sprintf(full_path, "%s/%s", plugin_dir, library);
    } else {
        full_path = strdup(library);
    }

    // Open the named library

    const char* fail_on_load = getenv("UDA_PLUGIN_FAIL_ON_LOAD");

    if (*pluginHandle == NULL) {
        if ((*pluginHandle = dlopen(full_path, RTLD_LOCAL | RTLD_LAZY)) == NULL) {
            err = 999;
            const char* errmsg = dlerror();
            UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library %s: %s\n", library, errmsg);
            if (fail_on_load != NULL) {
                addIdamError(SYSTEMERRORTYPE, __func__, err, "Cannot open the target shared library");
                addIdamError(SYSTEMERRORTYPE, __func__, err, errmsg);
            }
            return err;
        }
    }

    free(full_path);

    // Register the handle with the plugin manager: Close at server shut down only

    // Find the address of the required plugin function

    *(void**)(&fptr) = dlsym(*pluginHandle, symbol);

    char* errstr = dlerror();

    if (errstr == NULL) {
        *idamPlugin = (PLUGINFUNP)fptr;
    } else {
        err = 999;
        UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library %s: %s\n", library, errstr);
        if (fail_on_load != NULL) {
            addIdamError(CODEERRORTYPE, __func__, err, "Cannot locate the data reader with the target shared library");
            addIdamError(CODEERRORTYPE, __func__, err, errstr);
        }
        dlclose(pluginHandle);
        *pluginHandle = NULL;
        return err;
    }

    return err;
}