#include "bytesPlugin.h"

#include <plugins/uda_plugin_base.hpp>
#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>

#include <boost/filesystem.hpp>

#include "readBytesNonOptimally.h"

class BytesPlugin : public UDAPluginBase {
public:
    BytesPlugin();
    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int init(IDAM_PLUGIN_INTERFACE* plugin_interface) override { return 0; }
    int reset() override { return 0; }
};

BytesPlugin::BytesPlugin()
        : UDAPluginBase(
        "BYTES",
        1,
        "read",
        boost::filesystem::path(__FILE__).parent_path().append("help.txt").string()
)
{
    register_method("read", static_cast<UDAPluginBase::plugin_member_type>(&BytesPlugin::read));
}

int bytesPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static BytesPlugin plugin = {};
    return plugin.call(plugin_interface);
}

//----------------------------------------------------------------------------------------
// Add functionality here ....
int BytesPlugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    auto path = find_required_arg<std::string>(plugin_interface, "path");

    char c_path[MAXPATH];
    StringCopy(c_path, path.c_str(), MAXPATH);
    debug("expand_environment_variables!");
    expand_environment_variables(c_path);

    return readBytes(c_path, plugin_interface);
}
