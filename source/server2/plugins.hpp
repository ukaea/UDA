#pragma once

#ifndef UDA_SERVER_PLUGINS_HPP
#define UDA_SERVER_PLUGINS_HPP

#include <vector>
#include <boost/optional.hpp>

#include "udaDefines.h"
#include "udaPlugin.h"

enum pluginClass {
    UDA_PLUGIN_CLASS_UNKNOWN,
    UDA_PLUGIN_CLASS_FILE,     // File format access
    UDA_PLUGIN_CLASS_SERVER,   // Server protocol access
    UDA_PLUGIN_CLASS_FUNCTION, // Server-side function transformation
    UDA_PLUGIN_CLASS_DEVICE,   // Server to Server chaining, i.e. Pass the request to an external server
    UDA_PLUGIN_CLASS_OTHER
};

typedef int (*PLUGINFUNP)(UDA_PLUGIN_INTERFACE*); // Plugin function type

struct PluginData {
    char format[STRING_LENGTH];         // File format, or Function library or Server protocol or External Device name
    char library[STRING_LENGTH];        // external plugin shared library name (must be on Server library search path)
    char symbol[STRING_LENGTH];         // external plugin symbol name
    char method[STRING_LENGTH];         // Method to use for Data Readers (FILE Plugin Class)
    char extension[STRING_LENGTH];      // File Extension (Not Case sensitive)
    char deviceProtocol[STRING_LENGTH]; // Server protocol substitute for Device name
    char deviceHost[STRING_LENGTH];     // Server Host substitute for Device name
    char devicePort[STRING_LENGTH];     // Server Port substitute for Device name
    char desc[STRING_LENGTH];           // Description of the plugin
    char example[STRING_LENGTH];        // Examples of Use
    int request;                        // unique request ID
    unsigned short plugin_class;        // the plugin class: File, Server, Function, Device
    unsigned short external;            // Flag the plugin is accessed via a separate shared library
    unsigned short status;              // Plugin operational: external library opened or internal
    unsigned short is_private;          // The service is private and can NOT be used by external clients
    unsigned short cachePermission;     // The server's internal state may be dependent on previous calls
    // so the returned data are not suitable for caching on the client.
    // This is used to inform the client how to manage the returned data
    unsigned short interfaceVersion; // Maximum interface version the plugin is compliant with (Minimum is 1)
    void* pluginHandle;              // Plugin Library handle
    PLUGINFUNP idamPlugin;           // Plugin function address
};

typedef struct PluginList {
    int count;  // the number of plugins
    int mcount; // malloc count allocated
    PluginData* plugin;
} PLUGINLIST;

namespace uda {

class Plugins {
public:
    void init();

    void close();

    [[nodiscard]] PLUGINLIST as_plugin_list() const;

    [[nodiscard]] boost::optional<const PluginData&> find_by_format(const char* format) const;
    [[nodiscard]] boost::optional<const PluginData&> find_by_request(int request) const;

private:
    std::vector<PluginData> plugins_;

    void init_serverside_functions();

    void init_generic_plugin();

    bool initialised_ = false;

    void process_config_file(std::ifstream& conf_file);
};

}

#endif // UDA_SERVER_PLUGINS_HPP