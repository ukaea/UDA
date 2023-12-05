/*---------------------------------------------------------------
* v1 UDA Plugin Template: Standardised plugin design template, just add ...
*
* Input Arguments:    IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:        0 if the plugin functionality was successful
*            otherwise a Error Code is returned
*
* Standard functionality:
*
*    help    a description of what this plugin does together with a list of functions available
*
*    reset    frees all previously allocated heap, closes file handles and resets all static parameters.
*        This has the same functionality as setting the housekeeping directive in the plugin interface
*        data structure to TRUE (1)
*
*    init    Initialise the plugin: read all required data and process. Retain staticly for
*        future reference.
*
*---------------------------------------------------------------------------------------------------------------*/
#include "templatePlugin.h"

#include <plugins/uda_plugin_base.hpp>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>

#include <boost/filesystem.hpp>

class TemplatePlugin : public UDAPluginBase {
public:
    TemplatePlugin();
    int function(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int init(IDAM_PLUGIN_INTERFACE* plugin_interface) override { return 0; }
    int reset() override { return 0; }
};

TemplatePlugin::TemplatePlugin()
        : UDAPluginBase(
        "TEMPLATE",
        1,
        "read",
        boost::filesystem::path(__FILE__).parent_path().append("help.txt").string()
)
{
    register_method("function", static_cast<UDAPluginBase::plugin_member_type>(&TemplatePlugin::function));
}

int templatePlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static TemplatePlugin plugin = {};
    return plugin.call(plugin_interface);
}

namespace {

template <typename T>
std::string to_string(const std::vector<T>& array) {
    std::string result;
    const char* delim = "";
    for (const auto& el : array) {
        result += delim + std::to_string(el);
        delim = ", ";
    }
    return result;
}

} // anon namespace

//----------------------------------------------------------------------------------------
// Add functionality here ....
int TemplatePlugin::function(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    auto required = find_required_arg<std::string>(plugin_interface, "required");
    auto array = find_required_array_arg<double>(plugin_interface, "array");
    auto optional = find_arg<int>(plugin_interface, "optional");

    std::string result = std::string("Passed args: required=") + required
            + ", array=[" + to_string(array) + "]";
    if (optional) {
        result += ", optional=" + std::to_string(*optional) + ")";
    } else {
        result += ", optional=<NOT PASSED>)";
    }

    setReturnDataString(data_block, result.c_str(), "result of TemplatePlugin::function");

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}
