#include "bytesPlugin.h"

#include <boost/filesystem.hpp>
#include <uda/uda_plugin_base.hpp>

#include "readBytesNonOptimally.h"

UDA_PLUGIN_INFO UDA_PLUGIN_INFO_FUNCTION_NAME()
{
    UDA_PLUGIN_INFO info;
    info.name = "BYTES";
    info.version = "1.0";
    info.entry_function = "bytesPlugin";
    info.type = UDA_PLUGIN_CLASS_FUNCTION;
    info.extension = "";
    info.default_method = "read";
    info.description = "Data reader to access files as a block of bytes without interpretation";
    info.cache_mode = UDA_PLUGIN_CACHE_MODE_OK;
    info.is_private = false;
    info.interface_version = 1;
    return info;
}

class BytesPlugin : public UDAPluginBase
{
  public:
    BytesPlugin();
    int read(UDA_PLUGIN_INTERFACE* plugin_interface);
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}
};

BytesPlugin::BytesPlugin()
    : UDAPluginBase("BYTES", 1, "read", boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("read", static_cast<UDAPluginBase::plugin_member_type>(&BytesPlugin::read));
}

extern "C" int bytesPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static BytesPlugin plugin = {};
    return plugin.call(plugin_interface);
}

//----------------------------------------------------------------------------------------
// Add functionality here ....
int BytesPlugin::read(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto path = find_required_arg<std::string>(plugin_interface, "path");

    char c_path[UDA_MAX_PATH];
    strncpy(c_path, path.c_str(), UDA_MAX_PATH);
    c_path[UDA_MAX_PATH - 1] = '\0';
    debug(plugin_interface, "udaExpandEnvironmentalVariables!");
    udaExpandEnvironmentalVariables(c_path);

    return readBytes(c_path, plugin_interface);
}
