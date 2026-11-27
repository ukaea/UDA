#ifndef UDA_PLUGINS_UTILS_H
#define UDA_PLUGINS_UTILS_H

#include <clientserver/export.h>
#include <plugins/pluginStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check expanded_path against the UDA_ALLOWED_PATHS allow-list, logging through and recording errors on the given
 * plugin interface. Returns 0 if allowed, 999 otherwise ("Bad Path Provided <path>" for a rejected existing path,
 * "Provided File Path Not Found!" for a path that does not exist).
 */
LIBRARY_API int udaCheckAllowedPath(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* expanded_path);

/**
 * Legacy form of udaCheckAllowedPath(), kept for existing plugin binaries. It returns the same codes but its error
 * details are dropped (libuda_plugins has no error stack of its own), so the client only sees the server's generic
 * "Request failed with error code N" message. Use udaCheckAllowedPath().
 */
LIBRARY_API int check_allowed_path(const char* expanded_path);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_UTILS_H
