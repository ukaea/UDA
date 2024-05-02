#include "get_plugin_address.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <fmt/format.h>

#include "clientserver/errorLog.h"
#include "logging/logging.h"
#include "server_config.h"

using namespace uda::client_server;
using namespace uda::plugins;
using namespace uda::logging;

/**
 * Return the function address for plugin data readers located in external shared libraries
 *
 * @param pluginHandle
 * @param library the full file path name to the registered plugin shared library
 * @param symbol the name of the library api function to be called
 * @param pluginfunp the address of the library function
 * @return
 */
int uda::server::get_plugin_address(const Config& config, void** pluginHandle, const char* library, const char* symbol,
                                    PLUGINFUNP* pluginfunp)
{
    *pluginfunp = (PLUGINFUNP) nullptr;

    if (library[0] == '\0' || symbol[0] == '\0') {
        // Nothing to 'point' to! Is this an Error?
        return 0;
    }

    auto plugin_dir = config.get("plugin.directory");

    std::string full_path;
    if (plugin_dir) {
        full_path = fmt::format("{}/{}", plugin_dir.as<std::string>(), library);
    } else {
        full_path = strdup(library);
    }

    // Open the named library

    bool fail_on_load = config.get("plugin.fail_on_load").as_or_default(false);

    if (*pluginHandle == nullptr) {
        if ((*pluginHandle = dlopen(full_path.c_str(), RTLD_LOCAL | RTLD_LAZY)) == nullptr) {
            const char* errmsg = dlerror();
            UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library {}: {}", library, errmsg);
            if (fail_on_load) {
                UDA_ADD_ERROR(999, "Cannot open the target shared library");
                UDA_ADD_ERROR(999, errmsg);
            }
            return 999;
        }
    }

    // Register the handle with the plugin manager: Close at server shut down only

    // Find the address of the required plugin function

    int (*fptr)(UDA_PLUGIN_INTERFACE*);
    *(void**)(&fptr) = dlsym(*pluginHandle, symbol);

    char* errstr = dlerror();

    if (errstr == nullptr) {
        *pluginfunp = (PLUGINFUNP)fptr;
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library {}: {}", library, errstr);
        if (fail_on_load) {
            UDA_ADD_ERROR(999, "Cannot locate the data reader with the target shared library");
            UDA_ADD_ERROR(999, errstr);
        }
        dlclose(pluginHandle);
        *pluginHandle = nullptr;
        return 999;
    }

    return 0;
}
