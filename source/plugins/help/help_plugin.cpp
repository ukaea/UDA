#include "help_plugin.h"

#include <boost/filesystem.hpp>
#include <plugins/uda_plugin_base.hpp>

#include "accessors.h"
#include "initStructs.h"
#include "struct.h"
#include <fmt/format.h>

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

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    udaInitUserDefinedType(&usertype); // New structure definition
    udaInitCompoundField(&field);

    strcpy(usertype.name, "HELP_PING");
    strcpy(usertype.source, "idamServerHelp");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(HELP_PING); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;
    udaDefineField(&field, "seconds", "Server time in seconds from the epoch start", &offset, SCALARUINT);
    udaAddCompoundField(&usertype, field);
    udaDefineField(&field, "microseconds", "Server inter-second time in microseconds", &offset, SCALARUINT);
    udaAddCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    udaAddUserDefinedType(userdefinedtypelist, usertype);

    // assign the returned data structure

    auto data = (HELP_PING*)malloc(sizeof(HELP_PING));
    udaAddMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(HELP_PING), "HELP_PING"); // Register

    data->seconds = (unsigned int)serverTime.tv_sec;
    data->microseconds = (unsigned int)serverTime.tv_usec;

    // return to the client

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Local UDA server time");
    strcpy(data_block->data_label, "servertime");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "HELP_PING", 0);

    return 0;
}

int HelpPlugin::services(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    //======================================================================================
    // Plugin functionality
    int count;
    unsigned short target;

    const char* line = "\n------------------------------------------------------\n";

    // Document is a single block of chars

    std::string doc;

    // Total Number of registered plugins available

    const ENVIRONMENT* environment = plugin_interface->environment;

    const PLUGINLIST* pluginList = plugin_interface->pluginList;

    count = 0;
    for (int i = 0; i < pluginList->count; i++) {
        if (pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
            (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
             (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
            count++;
        }
    }

    doc = fmt::format("\nTotal number of registered plugins available: {}\n", count);

    for (int j = 0; j < 5; j++) {
        count = 0;
        doc += line;
        switch (j) {
            case 0: {
                target = UDA_PLUGIN_CLASS_FILE;

                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user)) &&
                        pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0') {
                        count++;
                    }
                }

                doc += fmt::format("\nNumber of plugins for data file formats: {}\n\n", count);

                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user)) &&
                        pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0') {
                        doc += fmt::format("File format:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Filename extension:\t{}\n", pluginList->plugin[i].extension);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 1: {
                target = UDA_PLUGIN_CLASS_FUNCTION;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Libraries: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Library name:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 2: {
                target = UDA_PLUGIN_CLASS_SERVER;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Data Servers: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Server name:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 3: {
                target = UDA_PLUGIN_CLASS_DEVICE;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for External Devices: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("External device name:\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 4: {
                target = UDA_PLUGIN_CLASS_OTHER;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Other data services: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Data service name:\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
        }
    }

    doc += "\n\n";

    return setReturnDataString(plugin_interface->data_block, doc.c_str(), "Description of UDA data access services");
}
