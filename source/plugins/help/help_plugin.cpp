#include "help_plugin.h"

#include <fmt/format.h>
#include <boost/filesystem.hpp>
#include <uda/uda_plugin_base.hpp>
#include <uda/portability.h>
#include <sys/time.h>

class HelpPlugin : public UDAPluginBase
{
  public:
    HelpPlugin();
    int ping(UDA_PLUGIN_INTERFACE* plugin_interface);
    int services(UDA_PLUGIN_INTERFACE* plugin_interface);
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}
};

HelpPlugin::HelpPlugin()
    : UDAPluginBase("HELP", 1, "read", boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("ping", static_cast<UDAPluginBase::plugin_member_type>(&HelpPlugin::ping));
    register_method("services", static_cast<UDAPluginBase::plugin_member_type>(&HelpPlugin::services));
}

int helpPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static HelpPlugin plugin = {};
    return plugin.call(plugin_interface);
}

int HelpPlugin::ping(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    //----------------------------------------------------------------------------------------

    // Ping: Timing
    struct timeval serverTime; // Local time in microseconds
    gettimeofday(&serverTime, nullptr);

    // define the returned data structure

    struct HELP_PING {
        unsigned int seconds;      // Server time in seconds
        unsigned int microseconds; // Server time in microseconds
    };
    typedef struct HELP_PING HELP_PING;

    int offset = 0;
    COMPOUNDFIELD* field1 = udaNewCompoundField("seconds", "Server time in seconds from the epoch start", &offset, SCALARUINT);
    COMPOUNDFIELD* field2 = udaNewCompoundField("microseconds", "Server inter-second time in microseconds", &offset, SCALARUINT);

    COMPOUNDFIELD* fields[] = { field1, field2 };
    USERDEFINEDTYPE* user_type = udaNewUserType("HELP_PING", "idamServerHelp", 0, 0, nullptr, sizeof(HELP_PING), 2, fields);

    udaAddUserType(plugin_interface, user_type);

    // assign the returned data structure

    auto data = (HELP_PING*)malloc(sizeof(HELP_PING));
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(HELP_PING), "HELP_PING");

    data->seconds = (unsigned int)serverTime.tv_sec;
    data->microseconds = (unsigned int)serverTime.tv_usec;

    // return to the client
    setReturnCompoundData(plugin_interface, (char*)data, "HELP_PING", nullptr);

    return 0;
}

int HelpPlugin::services(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const char* line = "\n------------------------------------------------------\n";

    // Document is a single block of chars

    std::string doc;

    // Total Number of registered plugins available
    
    int count = udaPluginPluginsCount(plugin_interface);

    doc = fmt::format("\nTotal number of registered plugins available: {}\n", count);

    for (int j = 0; j < 5; j++) {
        count = 0;
        doc += line;
        switch (j) {
            case 0: {
                doc += fmt::format("\nPlugins for data file formats: \n\n");

                for (int i = 0; i < count; i++) {
                    if (udaPluginCheckPluginClass(plugin_interface, i, "file")) {
                        doc += fmt::format("File format:\t\t{}\n", udaPluginPluginFormat(plugin_interface, i));
                        doc += fmt::format("Filename extension:\t{}\n", udaPluginPluginExtension(plugin_interface, i));
                        doc += fmt::format("Description:\t\t{}\n", udaPluginPluginDescription(plugin_interface, i));
                        doc += fmt::format("Example API call:\t{}\n\n", udaPluginPluginExample(plugin_interface, i));
                    }
                }
                break;
            }
            case 1: {
                doc += fmt::format("\nPlugins for functions:\n\n");
                
                for (int i = 0; i < count; i++) {
                    if (udaPluginCheckPluginClass(plugin_interface, i, "function")) {
                        doc += fmt::format("Library name:\t\t{}\n", udaPluginPluginFormat(plugin_interface, i));
                        doc += fmt::format("Description:\t\t{}\n", udaPluginPluginDescription(plugin_interface, i));
                        doc += fmt::format("Example API call:\t{}\n\n", udaPluginPluginExample(plugin_interface, i));
                    }
                }
                break;
            }
            case 2: {
                doc += fmt::format("\nPlugins for Data Servers\n\n");

                for (int i = 0; i < count; i++) {
                    if (udaPluginCheckPluginClass(plugin_interface, i, "server")) {
                        doc += fmt::format("Server name:\t\t{}\n", udaPluginPluginFormat(plugin_interface, i));
                        doc += fmt::format("Description:\t\t{}\n", udaPluginPluginDescription(plugin_interface, i));
                        doc += fmt::format("Example API call:\t{}\n\n", udaPluginPluginExample(plugin_interface, i));
                    }
                }
                break;
            }
            case 3: {
                doc += fmt::format("\nPlugins for External Devices:\n\n");

                for (int i = 0; i < count; i++) {
                    if (udaPluginCheckPluginClass(plugin_interface, i, "device")) {
                        doc += fmt::format("External device name:\t{}\n", udaPluginPluginFormat(plugin_interface, i));
                        doc += fmt::format("Description:\t\t{}\n", udaPluginPluginDescription(plugin_interface, i));
                        doc += fmt::format("Example API call:\t{}\n\n", udaPluginPluginExample(plugin_interface, i));
                    }
                }
                break;
            }
            case 4: {
                doc += fmt::format("\nNumber of plugins for Other data services:\n\n");

                for (int i = 0; i < count; i++) {
                    if (udaPluginCheckPluginClass(plugin_interface, i, "other")) {
                        doc += fmt::format("Data service name:\t{}\n", udaPluginPluginFormat(plugin_interface, i));
                        doc += fmt::format("Description:\t\t{}\n", udaPluginPluginDescription(plugin_interface, i));
                        doc += fmt::format("Example API call:\t{}\n\n", udaPluginPluginExample(plugin_interface, i));
                    }
                }
                break;
            }
        }
    }

    doc += "\n\n";

    return setReturnDataString(plugin_interface, doc.c_str(), "Description of UDA data access services");
}
