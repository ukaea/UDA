#include "bytesPlugin.h"

#include <boost/filesystem.hpp>
#include <uda/uda_plugin_base.hpp>

#include "readBytesNonOptimally.h"

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

int bytesPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
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
    strlcpy(c_path, path.c_str(), UDA_MAX_PATH);
    debug(plugin_interface, "udaExpandEnvironmentalVariables!");
    udaExpandEnvironmentalVariables(c_path);

    return readBytes(c_path, plugin_interface);
}
