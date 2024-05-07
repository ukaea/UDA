#include <boost/algorithm/string.hpp>
#include <dlfcn.h>
#include <fstream>
#include <sstream>
#include <string>

#include "get_plugin_address.hpp"
#include "plugins.hpp"

#include "clientserver/errorLog.h"
#include "clientserver/stringUtils.h"
#include "logging/logging.h"
#include "config/config.h"
#include "uda/plugins.h"

#define REQUEST_READ_START 1000

using namespace uda::client_server;
using namespace uda::plugins;
using namespace uda::logging;
using namespace uda::config;

namespace
{

void init_plugin_data(uda::plugins::PluginData* plugin)
{
    plugin->format[0] = '\0';
    plugin->library[0] = '\0';
    plugin->symbol[0] = '\0';
    plugin->extension[0] = '\0';
    plugin->desc[0] = '\0';
    plugin->example[0] = '\0';
    plugin->method[0] = '\0';
    plugin->deviceProtocol[0] = '\0';
    plugin->deviceHost[0] = '\0';
    plugin->devicePort[0] = '\0';
    plugin->request = REQUEST_READ_UNKNOWN;
    plugin->plugin_class = UDA_PLUGIN_CLASS_UNKNOWN;
    plugin->external = UDA_PLUGIN_INTERNAL;
    plugin->status = UDA_PLUGIN_NOT_OPERATIONAL;
    plugin->is_private = UDA_PLUGIN_PRIVATE;            // All services are private: Not accessible to external users
    plugin->cachePermission = UDA_PLUGIN_CACHE_DEFAULT; // Data are OK or Not for the Client to Cache
    plugin->interfaceVersion = 1;                       // Maximum Interface Version
    plugin->pluginHandle = nullptr;
    plugin->idamPlugin = nullptr;
}

int process_line(const std::string& line, uda::plugins::PluginData& plugin)
{
    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of(","), boost::token_compress_on);

    if (tokens.size() < 10) {
        UDA_ADD_ERROR(999, "Invalid line in plugin configuration file");
        return 999;
    }

    if (tokens.size() > 10) {
        std::vector<std::string> example(tokens.begin() + 9, tokens.end());
        tokens[9] = boost::algorithm::join(example, ",");
        tokens.resize(10);
    }

    int i = 0;
    for (auto& token : tokens) {
        uda::client_server::rtrim(token);
        uda::client_server::ltrim(token);
        auto ltoken = boost::to_lower_copy(token);

        switch (i) {
            case 0:
                // File Format or Server Protocol or Library name or Device name etc.
                strcpy(plugin.format, token.c_str());
                // If the Format or Protocol is Not unique, the plugin that is selected will be the first one
                // registered: others will be ignored.
                break;

            case 1: // Plugin class: File, Server, Function or Device
                plugin.plugin_class = UDA_PLUGIN_CLASS_FILE;
                if (ltoken == "server") {
                    plugin.plugin_class = UDA_PLUGIN_CLASS_SERVER;
                } else if (ltoken == "function") {
                    plugin.plugin_class = UDA_PLUGIN_CLASS_FUNCTION;
                } else if (ltoken == "file") {
                    plugin.plugin_class = UDA_PLUGIN_CLASS_FILE;
                } else if (ltoken == "device") {
                    plugin.plugin_class = UDA_PLUGIN_CLASS_DEVICE;
                }
                break;

            case 2:
                // Allow the same symbol (name of data access reader function or plugin entrypoint symbol) but from
                // different libraries!
                if (plugin.plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                    strcpy(plugin.symbol, token.c_str());
                    plugin.external = UDA_PLUGIN_EXTERNAL; // External (not linked) shared library

                    if (plugin.plugin_class == UDA_PLUGIN_CLASS_FILE) {
                        // Plugin method name using a dot syntax
                        char* p;
                        if ((p = strchr(plugin.symbol, '.')) != nullptr) {
                            p[0] = '\0';                  // Remove the method name from the symbol text
                            strcpy(plugin.method, &p[1]); // Save the method name
                        }
                    }

                } else {
                    // Device name Substitution protocol
                    strcpy(plugin.deviceProtocol, token.c_str());
                }
                break;

            case 3:
                // Server Host or Name of the shared library - can contain multiple plugin symbols so may not be unique
                if (plugin.plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                    strcpy(plugin.library, token.c_str());
                } else {
                    strcpy(plugin.deviceHost, token.c_str());
                }
                break;

            case 4:
                // File extension or Method Name or Port number
                // TODO: make extensions a list of valid extensions to minimise plugin duplication
                if (plugin.plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                    if (plugin.plugin_class == UDA_PLUGIN_CLASS_FILE) {
                        strcpy(plugin.extension, token.c_str());
                    } else if (token[0] != '*') {
                        // Ignore the placeholder character *
                        strcpy(plugin.method, token.c_str());
                    }
                } else {
                    strcpy(plugin.devicePort, token.c_str());
                }
                break;

            case 5:
                // Minimum Plugin Interface Version
                if (!token.empty()) {
                    plugin.interfaceVersion = (unsigned short)atoi(token.c_str());
                }
                break;

            case 6:
                // Permission to Cache returned values
                strcpy(plugin.desc, token.c_str());
                if (plugin.desc[0] != '\0' &&
                    (plugin.desc[0] == 'Y' || plugin.desc[0] == 'y' || plugin.desc[0] == 'T' || plugin.desc[0] == 't' ||
                     plugin.desc[0] == '1')) {
                    plugin.cachePermission = UDA_PLUGIN_OK_TO_CACHE; // True
                    plugin.desc[0] = '\0';
                } else {
                    plugin.cachePermission = UDA_PLUGIN_NOT_OK_TO_CACHE; // False
                }

                break;

            case 7:
                // Private or Public plugin - i.e. available to external users
                strcpy(plugin.desc, token.c_str());
                if (plugin.desc[0] != '\0' &&
                    (plugin.desc[0] == 'Y' || plugin.desc[0] == 'y' || plugin.desc[0] == 'T' || plugin.desc[0] == 't' ||
                     plugin.desc[0] == '1')) {
                    plugin.is_private = UDA_PLUGIN_PUBLIC;
                    plugin.desc[0] = '\0';
                }

                break;

            case 8:
                // Description
                strcpy(plugin.desc, token.c_str());
                break;

            case 9:
                // Example
                strcpy(plugin.example, token.c_str());
                break;

            default:
                break;
        }

        ++i;
    }

    return 0;
}

std::ifstream open_config_file(const Config& config)
{
    auto plugin_config = config.get("plugins.config");
    const char* default_file = "udaPlugins.conf"; // Default name
    std::string file_path;

    // Locate the plugin registration file

    if (plugin_config) {
        auto root = config.get("server.root");
        if (root) {
            file_path = root.as<std::string>() + "/" + default_file;
        } else {
            file_path = std::string{"./"} + default_file;
        }
    } else {
        file_path = plugin_config.as<std::string>();
    }

    // Read the registration file

    std::ifstream conf_file(file_path);
    return conf_file;
}

} // namespace

void uda::server::Plugins::init()
{
    if (_initialised) {
        return;
    }

    init_generic_plugin();
    init_serverside_functions();

    //----------------------------------------------------------------------------------------------------------------------
    // Read all other plugins registered via the server configuration file.

    std::ifstream conf_file = open_config_file(_config);
    if (!conf_file) {
        UDA_ADD_SYS_ERROR(strerror(errno));
        UDA_ADD_ERROR(999, "No Server Plugin Configuration File found!");
        return;
    }

    process_config_file(conf_file);

    UDA_LOG(UDA_LOG_INFO, "List of Plugins available");
    int i = 0;
    for (const auto& plugin : _plugins) {
        UDA_LOG(UDA_LOG_INFO, "[{}] {} {}", i, plugin.request, plugin.format);
        ++i;
    }

    _initialised = true;
}

void uda::server::Plugins::process_config_file(std::ifstream& conf_file)
{
    /*
    record format: csv, empty records ignored, comment begins #, max record size 1023;
    Organisation - context dependent - 10 fields
    Description field must not contain the csvChar character- ','
    A * (methodName) in field 5 is an ignorable placeholder

    1> Server plugins
    targetFormat,formatClass="server",librarySymbol,libraryName,methodName,interface,cachePermission,publicUse=,description,example
           2> Function library plugins
    targetFormat,formatClass="function",librarySymbol,libraryName,methodName,interface,cachePermission,publicUse,description,example
           3> File format
    targetFormat,formatClass="file",librarySymbol[.methodName],libraryName,fileExtension,interface,cachePermission,publicUse,description,example

           4> Internal Serverside function
           targetFormat,formatClass="function",librarySymbol="serverside",methodName,interface,cachePermission,publicUse,description,example
           5> External Device server re-direction
           targetFormat,formatClass="device",deviceProtocol,deviceHost,devicePort,interface,cachePermission,publicUse,description,example

    cachePermission and publicUse may use one of the following values: "Y|N,1|0,T|F,True|False"
    */

    uda::plugins::PluginData plugin = {};

    int rc = 0;
    static int offset = 0;

    std::string line;
    while (std::getline(conf_file, line)) {
        convert_non_printable(line);
        rtrim(line);
        ltrim(line);

        if (line.empty() || line == "#") {
            break;
        }

        init_plugin_data(&plugin);

        if ((rc = process_line(line, plugin)) != 0) {
            continue;
        }

        // Issue Unique request ID
        plugin.request = REQUEST_READ_START + offset++;
        plugin.pluginHandle = nullptr;              // Library handle: Not opened
        plugin.status = UDA_PLUGIN_NOT_OPERATIONAL; // Not yet available

        // Internal Serverside function ?
        if (plugin.plugin_class == UDA_PLUGIN_CLASS_FUNCTION && STR_IEQUALS(plugin.symbol, "serverside") &&
            plugin.library[0] == '\0') {
            strcpy(plugin.symbol, "SERVERSIDE");
            plugin.request = REQUEST_READ_GENERIC;
            plugin.external = UDA_PLUGIN_INTERNAL;
            plugin.status = UDA_PLUGIN_OPERATIONAL;
        }

        // Check this library has not already been opened: Preserve the library handle for use if already opened.

        int pluginID = -1;

        // States:
        // 1. library not opened: open library and locate symbol (Only if the Class is SERVER or FUNCTION or File)
        // 2. library opened, symbol not located: locate symbol
        // 3. library opened, symbol located: re-use

        int j = 0;
        for (auto& test : _plugins) {
            // External sources only
            if (test.external == UDA_PLUGIN_EXTERNAL && test.status == UDA_PLUGIN_OPERATIONAL &&
                test.pluginHandle != nullptr && STR_IEQUALS(test.library, plugin.library)) {

                // Library may contain different symbols

                if (STR_IEQUALS(test.symbol, plugin.symbol) && test.idamPlugin != nullptr) {
                    rc = 0;
                    plugin.idamPlugin = test.idamPlugin; // re-use
                } else {
                    // New symbol in opened library
                    if (plugin.plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                        rc = get_plugin_address(_config, &test.pluginHandle, // locate symbol
                                                test.library, plugin.symbol, &plugin.idamPlugin);
                    }
                }

                plugin.pluginHandle = test.pluginHandle;
                pluginID = j;
                break;
            }
            ++j;
        }

        if (pluginID == -1) { // open library and locate symbol
            if (plugin.plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                rc = get_plugin_address(_config, &plugin.pluginHandle, plugin.library, plugin.symbol,
                                        &plugin.idamPlugin);
            }
        }

        if (rc == 0) {
            plugin.status = UDA_PLUGIN_OPERATIONAL;
        }

        _plugins.push_back(plugin);
    }
}

void uda::server::Plugins::init_generic_plugin()
{
    //----------------------------------------------------------------------------------------------------------------------
    // Data Access Server Protocols

    // Generic

    PluginData plugin = {};

    strcpy(plugin.format, "GENERIC");
    plugin.request = REQUEST_READ_GENERIC;
    plugin.plugin_class = UDA_PLUGIN_CLASS_OTHER;
    plugin.is_private = UDA_PLUGIN_PUBLIC;
    strcpy(plugin.desc, "Generic Data Access request - no file format or server name specified, only the shot number");
    strcpy(plugin.example, R"(udaGetAPI("signal name", "12345"))");
    plugin.external = UDA_PLUGIN_INTERNAL;             // These are all linked as internal functions
    plugin.status = UDA_PLUGIN_OPERATIONAL;            // By default all these are available
    plugin.cachePermission = UDA_PLUGIN_CACHE_DEFAULT; // OK or not for Client and Server to Cache

    _plugins.push_back(plugin);
}

void uda::server::Plugins::init_serverside_functions()
{ //----------------------------------------------------------------------------------------------------------------------
    // Server-Side Functions

    auto names = {"SERVERSIDE", "SSIDE", "SS"};

    for (auto& name : names) {
        PluginData plugin = {};

        strcpy(plugin.format, name);
        plugin.request = REQUEST_READ_GENERIC;
        plugin.plugin_class = UDA_PLUGIN_CLASS_FUNCTION;
        strcpy(plugin.symbol, "SERVERSIDE");
        strcpy(plugin.desc, "Inbuilt Serverside functions");
        plugin.is_private = UDA_PLUGIN_PUBLIC;
        plugin.library[0] = '\0';
        plugin.pluginHandle = nullptr;
        plugin.external = UDA_PLUGIN_INTERNAL;             // These are all linked as internal functions
        plugin.status = UDA_PLUGIN_OPERATIONAL;            // By default all these are available
        plugin.cachePermission = UDA_PLUGIN_CACHE_DEFAULT; // OK or not for Client and Server to Cache

        _plugins.push_back(plugin);
    }
}

void uda::server::Plugins::close()
{
    for (auto& plugin : _plugins) {
        dlclose(plugin.pluginHandle);
    }
    _plugins.clear();
}

uda::plugins::PluginList uda::server::Plugins::as_plugin_list() const
{
    uda::plugins::PluginList plugin_list = {};
    plugin_list.plugin = (uda::plugins::PluginData*)_plugins.data();
    plugin_list.count = (int)_plugins.size();
    plugin_list.mcount = (int)_plugins.size();

    return plugin_list;
}

boost::optional<const uda::plugins::PluginData&> uda::server::Plugins::find_by_format(const char* format) const
{
    for (auto& plugin : _plugins) {
        if (STR_IEQUALS(plugin.format, format)) {
            return plugin;
        }
    }
    return {};
}

boost::optional<const uda::plugins::PluginData&> uda::server::Plugins::find_by_request(int request) const
{
    for (auto& plugin : _plugins) {
        if (plugin.request == request) {
            return plugin;
        }
    }
    return {};
}
