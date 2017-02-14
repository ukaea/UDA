#include "getPluginAddress.h"

#include <dlfcn.h>

#include <clientserver/idamErrors.h>

/**
 * Return the function address for plugin data readers located in external shared libraries
 *
 * @param pluginHandle
 * @param library the full file path name to the registered plugin shared library
 * @param symbol the name of the library api function to be called
 * @param idamPlugin the address of the library function
 * @return
 */
int getPluginAddress(void **pluginHandle, char *library, char *symbol, PLUGINFUNP *idamPlugin) {
    int err = 0;
    //void *handle = NULL;
    int (*fptr)(IDAM_PLUGIN_INTERFACE *);		// Pointer to a Plugin function with standard interface

    *idamPlugin = (PLUGINFUNP) NULL;			// Default

    if(library[0] == '\0' || symbol[0] == '\0') {		// Nothing to 'point' to! Is this an Error?
        return err;
    }

// Open the named library

    if(*pluginHandle == (void *)NULL) {
        if((*pluginHandle = dlopen(library, RTLD_LOCAL | RTLD_NOW)) == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "getPluginAddress: Cannot open the target shared library", err, dlerror());
            return err;
        }
    }

// Register the handle with the plugin manager: Close at server shut down only

// Find the address of the required plugin function

    *(void **)(&fptr) = dlsym(*pluginHandle, symbol);

    char * errstr = dlerror();

    if(errstr == NULL) {
        *idamPlugin = (PLUGINFUNP)fptr;
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE,
                     "getPluginAddress: Cannot locate the data reader with the target shared library", err, errstr);
        dlclose(pluginHandle);
        *pluginHandle = NULL;
        return err;
    }

    return err;
}
