#include "pluginInterface.h"

#include <algorithm>
#include <cstring>

#include <clientserver/errorLog.h>
#include <logging/logging.h>

void udaHostPluginLog(void* context, int level, const char* file, int line, const char* message)
{
    if (level < UDA_LOG_DEBUG || level > UDA_LOG_ACCESS || message == nullptr) {
        return;
    }
    auto name = static_cast<const char*>(context);
    size_t len = strlen(message);
    const char* newline = (len > 0 && message[len - 1] == '\n') ? "" : "\n";

    // Forwarded synchronously to the server's own logger, which applies the configured level, sinks and flushing.
    // The server logger never writes to the protocol streams.
    if (name != nullptr && name[0] != '\0') {
        udaLog((LOG_LEVEL)level, "%s:%d >> [%s] %s%s", file != nullptr ? file : "", line, name, message, newline);
    } else {
        udaLog((LOG_LEVEL)level, "%s:%d >> %s%s", file != nullptr ? file : "", line, message, newline);
    }
}

void udaSetPluginInterfaceTarget(IDAM_PLUGIN_INTERFACE* plugin_interface, const PLUGIN_DATA* plugin)
{
    int registered = UDA_PLUGIN_INTERFACE_VERSION;
    if (plugin != nullptr) {
        registered = plugin->interfaceVersion > 0 ? plugin->interfaceVersion : 1;
    }
    plugin_interface->interfaceVersion = (unsigned short)std::min(registered, UDA_PLUGIN_INTERFACE_VERSION);
    plugin_interface->log_context = plugin != nullptr ? (void*)plugin->format : nullptr;
}

void udaInitHostPluginInterface(IDAM_PLUGIN_INTERFACE* plugin_interface, const PLUGIN_DATA* plugin)
{
    memset(plugin_interface, 0, sizeof(IDAM_PLUGIN_INTERFACE));

    // Version 2 fields: harmless for version 1 plugins, which never read past error_stack
    plugin_interface->log_function = udaHostPluginLog;
    plugin_interface->log_level = (int)udaGetLogLevel();

    udaSetPluginInterfaceTarget(plugin_interface, plugin);
}

int udaCheckPluginInterfaceVersion(const PLUGIN_DATA* plugin)
{
    if (plugin == nullptr) {
        UDA_THROW_ERROR(999, "No plugin available for this request");
    }
    if (plugin->interfaceVersion > UDA_PLUGIN_INTERFACE_VERSION) {
        char msg[STRING_LENGTH];
        snprintf(msg, STRING_LENGTH,
                 "Plugin %s is registered for interface version %d but this server supports up to version %d",
                 plugin->format, plugin->interfaceVersion, UDA_PLUGIN_INTERFACE_VERSION);
        UDA_LOG(UDA_LOG_ERROR, "%s\n", msg);
        UDA_THROW_ERROR(999, msg);
    }
    return 0;
}

void udaCollectPluginErrors(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    unsigned int count = std::min(plugin_interface->error_stack.nerrors, (unsigned int)UDA_MAX_ERRORS);
    for (unsigned int i = 0; i < count; ++i) {
        const UDA_ERROR* error = &plugin_interface->error_stack.idamerror[i];
        addIdamError(error->type, error->location, error->code, error->msg);
    }
    plugin_interface->error_stack.nerrors = 0;
}
