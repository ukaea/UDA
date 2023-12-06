#include "hdf5plugin.h"

#include <plugins/uda_plugin_base.hpp>

#include "readHDF58.h"

class HDF5Plugin : public UDAPluginBase {
public:
    HDF5Plugin();
    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int hello(IDAM_PLUGIN_INTERFACE* plugin_interface)
    {
        return setReturnDataString(plugin_interface->data_block, "hello!", nullptr);
    }
    static int foo(IDAM_PLUGIN_INTERFACE* plugin_interface)
    {
        return setReturnDataString(plugin_interface->data_block, "foo!", nullptr);
    }
    void init(IDAM_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}
};

HDF5Plugin::HDF5Plugin()
    : UDAPluginBase("HDF5", 1, "read", "")
{
    register_method("read", static_cast<UDAPluginBase::plugin_member_type>(&HDF5Plugin::read));
    register_method("hello", static_cast<UDAPluginBase::plugin_member_type>(&HDF5Plugin::hello));
    register_function("foo", &HDF5Plugin::foo);
}

/**
 * Entry function
 */
extern int hdf5Plugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static HDF5Plugin plugin = {};
    return plugin.call(plugin_interface);
}

//----------------------------------------------------------------------------------------
// Read data from a HDF5 File


int HDF5Plugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_SOURCE* data_source = plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = plugin_interface->signal_desc;
    DATA_BLOCK* data_block = plugin_interface->data_block;

    auto file_path = find_required_arg<std::string>(plugin_interface, "file_path");
    auto cdf_path = find_required_arg<std::string>(plugin_interface, "cdf_path");

    strcpy(data_source->path, file_path.c_str());
    strcpy(signal_desc->signal_name, cdf_path.c_str());

    // Legacy data reader!
    int err = readHDF5(*data_source, *signal_desc, data_block);

    return err;
}
