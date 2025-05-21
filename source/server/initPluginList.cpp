#include "initPluginList.h"

#include <cerrno>

#include <cache/memcache.h>
#include <clientserver/stringUtils.h>
#include <server/serverPlugin.h>

#include "getPluginAddress.h"

void initPluginList(PLUGINLIST* plugin_list, ENVIRONMENT* environment)
{
    // initialise the Plugin List and Allocate heap for the list

    plugin_list->count = 0;
    plugin_list->plugin = (PLUGIN_DATA*)malloc(REQUEST_PLUGIN_MCOUNT * sizeof(PLUGIN_DATA));
    plugin_list->mcount = REQUEST_PLUGIN_MCOUNT;

    for (int i = 0; i < plugin_list->mcount; i++) {
        initPluginData(&plugin_list->plugin[i]);
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Data Access Server Protocols

    // Generic

    strcpy(plugin_list->plugin[plugin_list->count].format, "GENERIC");
    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_GENERIC;
    plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_OTHER;
    plugin_list->plugin[plugin_list->count].is_private = UDA_PLUGIN_PUBLIC;
    strcpy(plugin_list->plugin[plugin_list->count].desc,
           "Generic Data Access request - no file format or server name specified, only the shot number");
    strcpy(plugin_list->plugin[plugin_list->count].example, R"(udaGetAPI("signal name", "12345"))");
    allocPluginList(plugin_list->count++, plugin_list);

    //----------------------------------------------------------------------------------------------------------------------
    // Complete Common Registration

    for (int i = 0; i < plugin_list->count; i++) {
        plugin_list->plugin[i].external = UDA_PLUGIN_INTERNAL;        // These are all linked as internal functions
        plugin_list->plugin[i].status = UDA_PLUGIN_OPERATIONAL;        // By default all these are available
        plugin_list->plugin[i].cachePermission = UDA_PLUGIN_CACHE_DEFAULT;    // OK or not for Client and Server to Cache
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Server-Side Functions

    int pluginCount = plugin_list->count;        // Number of internal plugins before adding server-side

    strcpy(plugin_list->plugin[plugin_list->count].format, "SERVERSIDE");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "SSIDE");
    allocPluginList(plugin_list->count++, plugin_list);

    strcpy(plugin_list->plugin[plugin_list->count].format, "SS");
    allocPluginList(plugin_list->count++, plugin_list);

    for (int i = pluginCount; i < plugin_list->count; i++) {
        plugin_list->plugin[i].request = REQUEST_READ_GENERIC;
        plugin_list->plugin[i].plugin_class = UDA_PLUGIN_CLASS_FUNCTION;
        strcpy(plugin_list->plugin[i].symbol, "SERVERSIDE");
        strcpy(plugin_list->plugin[i].desc, "Inbuilt Serverside functions");
        plugin_list->plugin[i].is_private = UDA_PLUGIN_PUBLIC;
        plugin_list->plugin[i].library[0] = '\0';
        plugin_list->plugin[i].pluginHandle = nullptr;
        plugin_list->plugin[i].external = UDA_PLUGIN_INTERNAL;        // These are all linked as internal functions
        plugin_list->plugin[i].status = UDA_PLUGIN_OPERATIONAL;        // By default all these are available
        plugin_list->plugin[i].cachePermission = UDA_PLUGIN_CACHE_DEFAULT;    // OK or not for Client and Server to Cache
    }

    //----------------------------------------------------------------------------------------------------------------------
    // Read all other plugins registered via the server configuration file.

    {
        int rc = 0;
        static int offset = 0;
        char csvChar = ',';
        char buffer[STRING_LENGTH];
        char* root;
        char* config = getenv("UDA_PLUGIN_CONFIG");            // Server plugin configuration file
        FILE* conf = nullptr;
        const char* filename = "udaPlugins.conf";                // Default name
        char* work = nullptr, * csv, * next, * p;

        // Locate the plugin registration file

        int lstr;
        if (config == nullptr) {
            root = getenv("UDA_SERVERROOT");                // Where udaPlugins.conf is located by default
            if (root == nullptr) {
                lstr = (int)strlen(filename) + 3;
                work = (char*)malloc(lstr * sizeof(char));
                snprintf(work, lstr, "./%s", filename);            // Default ROOT is the server's Working Directory
            } else {
                lstr = (int)strlen(filename) + (int)strlen(root) + 2;
                work = (char*)malloc(lstr * sizeof(char));
                snprintf(work, lstr, "%s/%s", root, filename);
            }
        } else {
            lstr = (int)strlen(config) + 1;
            work = (char*)malloc(lstr * sizeof(char));            // Alternative File Name and Path
            strcpy(work, config);
        }

        // Read the registration file

        errno = 0;
        if ((conf = fopen(work, "r")) == nullptr || errno != 0) {
            ADD_SYS_ERROR(strerror(errno));
            ADD_ERROR(999, "No Server Plugin Configuration File found!");
            if (conf != nullptr) {
                fclose(conf);
            }
            free(work);
            return;
        }

        free(work);

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

        while (fgets(buffer, STRING_LENGTH, conf) != nullptr) {
            convertNonPrintable2(buffer);
            LeftTrimString(TrimString(buffer));
            do {
                if (buffer[0] == '#') break;
                if (strlen(buffer) == 0) break;
                next = buffer;
                initPluginData(&plugin_list->plugin[plugin_list->count]);
                for (int i = 0; i < 10; i++) {
                    csv = strchr(next, csvChar);                // Split the string
                    if (csv != nullptr && i <= 8)
                        csv[0] = '\0';            // Extract the sub-string ignoring the example - has a comma within text
                    LeftTrimString(TrimString(next));
                    switch (i) {

                        case 0:
                            // File Format or Server Protocol or Library name or Device name etc.
                            strcpy(plugin_list->plugin[plugin_list->count].format, LeftTrimString(next));
                            // If the Format or Protocol is Not unique, the plugin that is selected will be the first one registered: others will be ignored.
                            break;

                        case 1:    // Plugin class: File, Server, Function or Device
                            plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_FILE;
                            if (STR_IEQUALS(LeftTrimString(next), "server")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_SERVER;
                            } else if (STR_IEQUALS(LeftTrimString(next), "function")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_FUNCTION;
                            } else if (STR_IEQUALS(LeftTrimString(next), "file")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_FILE;
                            } else if (STR_IEQUALS(LeftTrimString(next), "device")) {
                                plugin_list->plugin[plugin_list->count].plugin_class = UDA_PLUGIN_CLASS_DEVICE;
                            }
                            break;

                        case 2:
                            // Allow the same symbol (name of data access reader function or plugin entrypoint symbol) but from different libraries!
                            if (plugin_list->plugin[plugin_list->count].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                                strcpy(plugin_list->plugin[plugin_list->count].symbol, LeftTrimString(next));
                                plugin_list->plugin[plugin_list->count].external = UDA_PLUGIN_EXTERNAL;        // External (not linked) shared library

                                if (plugin_list->plugin[plugin_list->count].plugin_class == UDA_PLUGIN_CLASS_FILE) {
                                    // Plugin method name using a dot syntax
                                    if ((p = strchr(plugin_list->plugin[plugin_list->count].symbol, '.')) != nullptr) {
                                        p[0] = '\0';                                // Remove the method name from the symbol text
                                        strcpy(plugin_list->plugin[plugin_list->count].method, &p[1]);        // Save the method name
                                    }
                                }

                            } else {
                                // Device name Substitution protocol
                                strcpy(plugin_list->plugin[plugin_list->count].deviceProtocol, LeftTrimString(next));
                            }
                            break;

                        case 3:    // Server Host or Name of the shared library - can contain multiple plugin symbols so may not be unique
                            if (plugin_list->plugin[plugin_list->count].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                                strcpy(plugin_list->plugin[plugin_list->count].library, LeftTrimString(next));
                            } else {
                                strcpy(plugin_list->plugin[plugin_list->count].deviceHost, LeftTrimString(next));
                            }
                            break;

                        case 4:    // File extension or Method Name or Port number
                            // TODO: make extensions a list of valid extensions to minimise plugin duplication
                            if (plugin_list->plugin[plugin_list->count].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                                if (plugin_list->plugin[plugin_list->count].plugin_class == UDA_PLUGIN_CLASS_FILE) {
                                    strcpy(plugin_list->plugin[plugin_list->count].extension, next);
                                } else if (next[0] != '*') {
                                    // Ignore the placeholder character *
                                    strcpy(plugin_list->plugin[plugin_list->count].method, next);
                                }
                            } else {
                                strcpy(plugin_list->plugin[plugin_list->count].devicePort, LeftTrimString(next));
                            }
                            break;

                        case 5:    // Minimum Plugin Interface Version
                            if (strlen(next) > 0) {
                                plugin_list->plugin[plugin_list->count].interfaceVersion = (unsigned short)atoi(next);
                            }
                            break;

                        case 6:    // Permission to Cache returned values

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            if (plugin_list->plugin[plugin_list->count].desc[0] != '\0' && (
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'Y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'T' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 't' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == '1')) {
                                plugin_list->plugin[plugin_list->count].cachePermission = UDA_PLUGIN_OK_TO_CACHE;      // True
                                plugin_list->plugin[plugin_list->count].desc[0] = '\0';
                            } else
                                plugin_list->plugin[plugin_list->count].cachePermission = UDA_PLUGIN_NOT_OK_TO_CACHE;   // False

                            break;

                        case 7:    // Private or Public plugin - i.e. available to external users

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            if (plugin_list->plugin[plugin_list->count].desc[0] != '\0' && (
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'Y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'y' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 'T' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == 't' ||
                                    plugin_list->plugin[plugin_list->count].desc[0] == '1')) {
                                plugin_list->plugin[plugin_list->count].is_private = UDA_PLUGIN_PUBLIC;
                                plugin_list->plugin[plugin_list->count].desc[0] = '\0';
                            }

                            break;

                        case 8:    // Description

                            strcpy(plugin_list->plugin[plugin_list->count].desc, LeftTrimString(next));
                            break;

                        case 9:    // Example

                            LeftTrimString(next);
                            p = strchr(next, '\n');
                            if (p != nullptr) {
                                p[0] = '\0';
                            }
                            strcpy(plugin_list->plugin[plugin_list->count].example, LeftTrimString(next));
                            break;

                        default:
                            break;

                    }
                    if (csv != nullptr) next = &csv[1];    // Next element starting point
                }

                // Issue Unique request ID
                plugin_list->plugin[plugin_list->count].request = REQUEST_READ_START + offset++;
                plugin_list->plugin[plugin_list->count].pluginHandle = nullptr;            // Library handle: Not opened
                plugin_list->plugin[plugin_list->count].status = UDA_PLUGIN_NOT_OPERATIONAL;  // Not yet available

                // Internal Serverside function ?

                if (plugin_list->plugin[plugin_list->count].plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
                    STR_IEQUALS(plugin_list->plugin[plugin_list->count].symbol, "serverside") &&
                    plugin_list->plugin[plugin_list->count].library[0] == '\0') {
                    strcpy(plugin_list->plugin[plugin_list->count].symbol, "SERVERSIDE");
                    plugin_list->plugin[plugin_list->count].request = REQUEST_READ_GENERIC;
                    plugin_list->plugin[plugin_list->count].external = UDA_PLUGIN_INTERNAL;
                    plugin_list->plugin[plugin_list->count].status = UDA_PLUGIN_OPERATIONAL;
                }

                // Check this library has not already been opened: Preserve the library handle for use if already opened.

                int pluginID = -1;

                // States:
                // 1. library not opened: open library and locate symbol (Only if the Class is SERVER or FUNCTION or File)
                // 2. library opened, symbol not located: locate symbol
                // 3. library opened, symbol located: re-use

                for (int j = pluginCount; j < plugin_list->count - 1; j++) {            // External sources only
                    if (plugin_list->plugin[j].external == UDA_PLUGIN_EXTERNAL &&
                        plugin_list->plugin[j].status == UDA_PLUGIN_OPERATIONAL &&
                        plugin_list->plugin[j].pluginHandle != nullptr &&
                        STR_IEQUALS(plugin_list->plugin[j].library, plugin_list->plugin[plugin_list->count].library)) {

                        // Library may contain different symbols

                        if (STR_IEQUALS(plugin_list->plugin[j].symbol,
                                        plugin_list->plugin[plugin_list->count].symbol) &&
                            plugin_list->plugin[j].idamPlugin != nullptr) {
                            rc = 0;
                            plugin_list->plugin[plugin_list->count].idamPlugin = plugin_list->plugin[j].idamPlugin;    // re-use
                        } else {

                            // New symbol in opened library

                            if (plugin_list->plugin[plugin_list->count].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                                rc = getPluginAddress(
                                        &plugin_list->plugin[j].pluginHandle,                // locate symbol
                                        plugin_list->plugin[j].library,
                                        plugin_list->plugin[plugin_list->count].symbol,
                                        &plugin_list->plugin[plugin_list->count].idamPlugin);
                            }
                        }

                        plugin_list->plugin[plugin_list->count].pluginHandle = plugin_list->plugin[j].pluginHandle;
                        pluginID = j;
                        break;
                    }
                }

                if (pluginID == -1) {                                    // open library and locate symbol
                    if (plugin_list->plugin[plugin_list->count].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                        rc = getPluginAddress(&plugin_list->plugin[plugin_list->count].pluginHandle,
                                              plugin_list->plugin[plugin_list->count].library,
                                              plugin_list->plugin[plugin_list->count].symbol,
                                              &plugin_list->plugin[plugin_list->count].idamPlugin);
                    }
                }

                if (rc == 0) {
                    plugin_list->plugin[plugin_list->count].status = UDA_PLUGIN_OPERATIONAL;
                }

                allocPluginList(plugin_list->count++, plugin_list);

            } while (0);
        }

        fclose(conf);
    }
}
