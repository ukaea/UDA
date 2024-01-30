/*---------------------------------------------------------------
 * v1 UDA Plugin Template: Standardised plugin design template, just add ...
 *
 * Input Arguments:    UDA_PLUGIN_INTERFACE *plugin_interface
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

#include "clientserver/initStructs.h"
#include "clientserver/stringUtils.h"
#include "include/uda/uda_plugin_base.hpp"

#include <boost/filesystem.hpp>

class TemplatePlugin : public UDAPluginBase
{
  public:
    TemplatePlugin();
    int function(UDA_PLUGIN_INTERFACE* plugin_interface);
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}
};

TemplatePlugin::TemplatePlugin()
    : UDAPluginBase("TEMPLATE", 1, "function",
                    boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("function", static_cast<UDAPluginBase::plugin_member_type>(&TemplatePlugin::function));
}

int templatePlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static TemplatePlugin plugin = {};
    return plugin.call(plugin_interface);
}

namespace
{

template <typename T> std::string to_string(const std::vector<T>& array)
{
    std::string result;
    const char* delim = "";
    for (const auto& el : array) {
        result += delim + std::to_string(el);
        delim = ", ";
    }
    return result;
}

} // namespace

//----------------------------------------------------------------------------------------
// Add functionality here ....
int TemplatePlugin::function(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto required = find_required_arg<std::string>(plugin_interface, "required");
    auto array = find_required_array_arg<double>(plugin_interface, "array");
    auto optional = find_arg<int>(plugin_interface, "optional");

    std::string optional_str = optional ? std::to_string(*optional) : "<NOT PASSED>";
    std::string result =
        fmt::format("Passed args: required={}, array=[{}], optional={}", required, to_string(array), optional_str);

    setReturnDataString(plugin_interface, result.c_str(), "result of TemplatePlugin::function");
    setReturnDataLabel(plugin_interface, "");
    setReturnDataUnits(plugin_interface, "");

    return 0;
}
