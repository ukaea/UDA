#ifndef UDA_SERVER_PLUGININTERFACE_H
#define UDA_SERVER_PLUGININTERFACE_H

#include <plugins/pluginStructs.h>

/**
 * Host-side helpers for calling plugins. Every site that builds an IDAM_PLUGIN_INTERFACE and calls a plugin entry
 * point should go through these, so that no field is left uninitialised, the interface version is negotiated in
 * one place and plugin errors and diagnostics reach the server's own error stack and log sinks.
 */

/**
 * Value-initialise every field of the interface, negotiate the interface version for the given plugin and install
 * the host logging sink. plugin may be NULL, in which case the host's maximum version is used.
 *
 * The plugin is given min(registered version, UDA_PLUGIN_INTERFACE_VERSION), so version 1 plugins keep working.
 */
void udaInitHostPluginInterface(IDAM_PLUGIN_INTERFACE* plugin_interface, const PLUGIN_DATA* plugin);

/**
 * Re-target an already initialised interface at a specific plugin: negotiates the interface version and sets the
 * logging context, leaving every other field untouched.
 */
void udaSetPluginInterfaceTarget(IDAM_PLUGIN_INTERFACE* plugin_interface, const PLUGIN_DATA* plugin);

/**
 * Check the plugin's registered interface version is supported by this host before calling its entry point.
 *
 * @return 0 if supported, otherwise 999 with an error recorded on the server error stack
 */
int udaCheckPluginInterfaceVersion(const PLUGIN_DATA* plugin);

/**
 * Transfer the records on the interface's error stack to the server error stack (once), then clear them.
 */
void udaCollectPluginErrors(IDAM_PLUGIN_INTERFACE* plugin_interface);

/**
 * Host logging sink installed by udaInitHostPluginInterface(). context is the plugin's format name (or NULL).
 */
void udaHostPluginLog(void* context, int level, const char* file, int line, const char* message);

/**
 * Create a stand-alone host plugin interface for request, with the server environment and a freshly initialised
 * plugin list. Free with udaFreePluginInterface().
 */
IDAM_PLUGIN_INTERFACE* udaCreatePluginInterface(const char* request);
void udaFreePluginInterface(IDAM_PLUGIN_INTERFACE* plugin_interface);

#endif // UDA_SERVER_PLUGININTERFACE_H
