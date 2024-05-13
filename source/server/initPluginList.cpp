#include "initPluginList.h"

#include <cerrno>
#include <fmt/format.h>
#include <dlfcn.h>

#include "cache/memcache.hpp"
#include "clientserver/errorLog.h"
#include "common/stringUtils.h"
#include "server/serverPlugin.h"
#include "uda/plugins.h"

#include "getPluginAddress.h"

using namespace uda::client_server;
using namespace uda::plugins;

void uda::server::initPluginList(std::vector<PluginData>& plugin_list)
{
    //----------------------------------------------------------------------------------------------------------------------
    // Data Access Server Protocols

    // Generic

    PluginData plugin_data = {};
    plugin_data.name = "GENERIC";
    plugin_data.type = UDA_PLUGIN_CLASS_OTHER;
    plugin_data.is_private = UDA_PLUGIN_PUBLIC;
    plugin_data.description = "Generic Data Access request - no file format or server name specified, only the shot number";
    plugin_list.emplace_back(std::move(plugin_data));

    //----------------------------------------------------------------------------------------------------------------------
    // Server-Side Functions

    plugin_data = {};
    plugin_data.name = "SERVERSIDE";
    plugin_data.type = UDA_PLUGIN_CLASS_FUNCTION;
    plugin_data.entry_func_name = "SERVERSIDE";
    plugin_data.is_private = UDA_PLUGIN_PUBLIC;
    plugin_data.description = "Inbuilt Serverside functions";
    plugin_data.cache_mode = UDA_PLUGIN_CACHE_MODE_NONE;
    plugin_list.emplace_back(std::move(plugin_data));

    //----------------------------------------------------------------------------------------------------------------------
    // Read all other plugins registered via the server configuration file.

    {
        int rc = 0;
        char csv_char = ',';
        char buffer[STRING_LENGTH];
        char* root;
        char* config = getenv("UDA_PLUGIN_CONFIG"); // Server plugin configuration file
        FILE* conf = nullptr;
        const char* filename = "udaPlugins.conf"; // Default name
        std::string work;

        // Locate the plugin registration file

        if (config == nullptr) {
            root = getenv("UDA_SERVERROOT"); // Where udaPlugins.conf is located by default
            if (root == nullptr) {
                work = fmt::format("./{}", filename); // Default ROOT is the server's Working Directory
            } else {
                work = fmt::format("{}/{}", root, filename);
            }
        } else {
            work = config; // Alternative File Name and Path
        }

        // Read the registration file

        errno = 0;
        if ((conf = fopen(work.c_str(), "r")) == nullptr || errno != 0) {
            UDA_ADD_SYS_ERROR(strerror(errno));
            UDA_ADD_ERROR(999, "No Server Plugin Configuration File found!");
            if (conf != nullptr) {
                fclose(conf);
            }
            return;
        }

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

        while (fgets(buffer, STRING_LENGTH - 1, conf) != nullptr) {
            convert_non_printable2(buffer);
            left_trim_string(trim_string(buffer));
            do {
                if (buffer[0] == '#') {
                    break;
                }
                if (strlen(buffer) == 0) {
                    break;
                }
                char* next = buffer;
                plugin_data = {};
                for (int i = 0; i < 10; i++) {
                    char* csv = strchr(next, csv_char); // Split the string
                    if (csv != nullptr && i <= 8) {
                        csv[0] = '\0'; // Extract the sub-string ignoring the example - has a comma within text
                    }
                    left_trim_string(trim_string(next));
                    switch (i) {

                        case 0:
                            // File Format or Server Protocol or Library name or Device name etc.
                            plugin_data.name = left_trim_string(next);
                            // If the Format or Protocol is Not unique, the plugin that is selected will be the first
                            // one registered: others will be ignored.
                            break;

                        case 1: // Plugin class: File, Server, Function or Device
                            plugin_data.type = UDA_PLUGIN_CLASS_FILE;
                            if (STR_IEQUALS(left_trim_string(next), "server")) {
                                plugin_data.type = UDA_PLUGIN_CLASS_SERVER;
                            } else if (STR_IEQUALS(left_trim_string(next), "function")) {
                                plugin_data.type = UDA_PLUGIN_CLASS_FUNCTION;
                            } else if (STR_IEQUALS(left_trim_string(next), "file")) {
                                plugin_data.type = UDA_PLUGIN_CLASS_FILE;
                            } else if (STR_IEQUALS(left_trim_string(next), "device")) {
                                plugin_data.type = UDA_PLUGIN_CLASS_DEVICE;
                            }
                            break;

                        case 2:
                            // Allow the same symbol (name of data access reader function or plugin entrypoint symbol)
                            // but from different libraries!
                            if (plugin_data.type != UDA_PLUGIN_CLASS_DEVICE) {
                                plugin_data.entry_func_name = left_trim_string(next);

                                if (plugin_data.type == UDA_PLUGIN_CLASS_FILE) {
                                    // Plugin method name using a dot syntax
                                    auto pos = plugin_data.entry_func_name.find('.');
                                    if (pos != std::string::npos) {
                                        plugin_data.default_method = plugin_data.entry_func_name.substr(pos + 1);
                                        plugin_data.entry_func_name = plugin_data.entry_func_name.substr(0, pos);
                                    }
                                }

                            }
                            break;

                        case 3: // Server Host or Name of the shared library - can contain multiple plugin symbols so
                                // may not be unique
                            if (plugin_data.type != UDA_PLUGIN_CLASS_DEVICE) {
                                plugin_data.library_name = left_trim_string(next);
                            }
                            break;

                        case 4: // File extension or Method Name or Port number
                            // TODO: make extensions a list of valid extensions to minimise plugin duplication
                            if (plugin_data.type != UDA_PLUGIN_CLASS_DEVICE) {
                                if (plugin_data.type == UDA_PLUGIN_CLASS_FILE) {
                                    plugin_data.extension = next;
                                } else if (next[0] != '*') {
                                    // Ignore the placeholder character *
                                    plugin_data.default_method = next;
                                }
                            }
                            break;

                        case 5: // Minimum Plugin Interface Version
                            if (strlen(next) > 0) {
                                plugin_data.interface_version = (unsigned short)atoi(next);
                            }
                            break;

                        case 6: // Permission to Cache returned values
                        {
                            std::string val = left_trim_string(next);
                            if (!val.empty()
                                    && (val[0] == 'Y' || val[0] == 'y' || val[0] == 'T' || val[0] == 't' || val[0] == '1')) {
                                plugin_data.cache_mode = UDA_PLUGIN_CACHE_MODE_OK;
                            } else {
                                plugin_data.cache_mode = UDA_PLUGIN_CACHE_MODE_NONE;
                            }
                        }
                            break;

                        case 7: // Private or Public plugin - i.e. available to external users
                        {
                            std::string val = left_trim_string(next);
                            if (!val.empty()
                                && (val[0] == 'Y' || val[0] == 'y' || val[0] == 'T' || val[0] == 't' || val[0] == '1')) {
                                plugin_data.is_private = UDA_PLUGIN_PUBLIC;
                            } else {
                                plugin_data.is_private = UDA_PLUGIN_PRIVATE;
                            }
                        }
                            break;

                        case 8: // Description
                            plugin_data.description = left_trim_string(next);
                            break;

                        case 9: {
                            // Example
                            left_trim_string(next);
                            char* p = strchr(next, '\n');
                            if (p != nullptr) {
                                p[0] = '\0';
                            }
                            plugin_data.example = left_trim_string(next);
                            break;
                        }

                        default:
                            break;
                    }
                    if (csv != nullptr) {
                        next = &csv[1]; // Next element starting point
                    }
                }

                // Check this library has not already been opened: Preserve the library handle for use if already
                // opened.

                int plugin_id = -1;

                // States:
                // 1. library not opened: open library and locate symbol (Only if the Class is SERVER or FUNCTION or
                // File)
                // 2. library opened, symbol not located: locate symbol
                // 3. library opened, symbol located: re-use

                for (const auto& plugin : plugin_list) {
                    if (plugin.handle != nullptr && plugin.library_name == plugin_data.library_name) {

                        // Library may contain different symbols
                        if (plugin.entry_func_name == plugin_data.entry_func_name && plugin.entry_func != nullptr) {
                            rc = 0;
                            plugin_data.entry_func = plugin.entry_func; // re-use
                        } else {

                            // New symbol in opened library

                            if (plugin_data.type != UDA_PLUGIN_CLASS_DEVICE) {
                                auto handle = plugin.handle.get();
                                rc = getPluginAddress(&handle, // locate symbol
                                                      plugin.library_name.c_str(),
                                                      plugin_data.entry_func_name.c_str(),
                                                      &plugin_data.entry_func);
                            }
                        }

                        break;
                    }
                }

                if (plugin_id == -1) { // open library and locate symbol
                    if (plugin_data.type != UDA_PLUGIN_CLASS_DEVICE) {
                        void* handle = nullptr;
                        rc = getPluginAddress(&handle,
                                              plugin_data.library_name.c_str(),
                                              plugin_data.entry_func_name.c_str(),
                                              &plugin_data.entry_func);
                        plugin_data.handle = {handle, dl_close};
                    }
                }

                if (rc == 0) {
                    plugin_list.emplace_back(std::move(plugin_data));
                }
            } while (0);
        }

        fclose(conf);
    }
}
