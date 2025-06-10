#include "getPluginAddress.h"

#include <dlfcn.h>
#include <cstdlib>

/**
 * Return the function address for plugin data readers located in external shared libraries
 *
 * @param pluginHandle
 * @param library the full file path name to the registered plugin shared library
 * @param symbol the name of the library api function to be called
 * @param pluginfunp the address of the library function
 * @return
 */
int getPluginAddress(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* pluginfunp)
{
    *pluginfunp = (PLUGINFUNP)nullptr;

    if (library[0] == '\0' || symbol[0] == '\0') {
        // Nothing to 'point' to! Is this an Error?
        return 0;
    }

    const char* plugin_dir = getenv("UDA_PLUGIN_DIR");
    char* full_path;
    if (plugin_dir != nullptr) {
        const auto lstr = strlen(plugin_dir) + strlen(library) + 2;
        full_path = (char*)malloc(lstr);
        snprintf(full_path, lstr, "%s/%s", plugin_dir, library);
    } else {
        full_path = strdup(library);
    }

    // Open the named library

    const char* fail_on_load = getenv("UDA_PLUGIN_FAIL_ON_LOAD");

    if (*pluginHandle == nullptr) {
        if ((*pluginHandle = dlopen(full_path, RTLD_LOCAL | RTLD_LAZY)) == nullptr) {
            const char* errmsg = dlerror();
            UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library %s: %s\n", library, errmsg);
            if (fail_on_load != nullptr) {
                ADD_ERROR(999, "Cannot open the target shared library");
                ADD_ERROR(999, errmsg);
            }
            return 999;
        }
    }

    free(full_path);

    // Register the handle with the plugin manager: Close at server shut down only

    // Find the address of the required plugin function

    int (* fptr)(IDAM_PLUGIN_INTERFACE*);
    *(void**)(&fptr) = dlsym(*pluginHandle, symbol);

    char* errstr = dlerror();

    if (errstr == nullptr) {
        *pluginfunp = (PLUGINFUNP)fptr;
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library %s: %s\n", library, errstr);
        if (fail_on_load != nullptr) {
            ADD_ERROR(999, "Cannot locate the data reader with the target shared library");
            ADD_ERROR(999, errstr);
        }
        dlclose(pluginHandle);
        *pluginHandle = nullptr;
        return 999;
    }

    return 0;
}
