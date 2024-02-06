#include "include/uda/uda_plugin_base.hpp"
#include "clientserver/stringUtils.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

int UDAPluginBase::call(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    try {
        if (udaPluginCheckInterfaceVersion(plugin_interface, interface_version_)) {
            error(plugin_interface, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        }

        udaPluginSetVersion(plugin_interface, version_);

        do_reset();
        do_init(plugin_interface);

        //----------------------------------------------------------------------------------------
        // Plugin Functions
        //----------------------------------------------------------------------------------------

        std::string function = get_function(plugin_interface);

        int rc = 0;

        if (function_map_.find(function) != function_map_.end()) {
            rc = function_map_.at(function)(plugin_interface);
        } else if (method_map_.find(function) != method_map_.end()) {
            auto fn = method_map_.at(function);
            rc = (this->*fn)(plugin_interface);
        } else {
            error(plugin_interface, "Unknown function requested");
        }
        return rc;
    } catch (std::exception& ex) {
        debug(plugin_interface, ex.what());
        return 1;
    }
}

std::string UDAPluginBase::get_function(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return boost::to_lower_copy<std::string>(udaPluginFunction(plugin_interface));
}

void UDAPluginBase::do_init(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    std::string function = get_function(plugin_interface);
    if (!init_ || (function == "init" || function == "initialise")) {
        init(plugin_interface);
        init_ = true;
    }
}

void UDAPluginBase::do_reset()
{
    if (!init_) {
        // Not previously initialised: Nothing to do!
        return;
    }

    reset();
    init_ = false;
}

int UDAPluginBase::help(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    std::string desc = name_ + ": help = description of this plugin";

    if (help_file_.empty()) {
        const char* help = "No help available";
        return udaPluginReturnDataStringScalar(plugin_interface, help, desc.c_str());
    }

    auto path = boost::filesystem::path(help_file_);
    if (!boost::filesystem::exists(path)) {
        auto help = fmt::format("help file {} does not exist", path.string());
        return udaPluginReturnDataStringScalar(plugin_interface, help.c_str(), desc.c_str());
    }

    std::ifstream help_file{path.string()};
    std::stringstream buffer;
    buffer << help_file.rdbuf();
    auto help = buffer.str();

    return udaPluginReturnDataStringScalar(plugin_interface, help.c_str(), desc.c_str());
}

int UDAPluginBase::version(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaPluginReturnDataIntScalar(plugin_interface, version_, "Plugin version number");
}

int UDAPluginBase::build_date(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaPluginReturnDataStringScalar(plugin_interface, __DATE__, "Plugin build date");
}

int UDAPluginBase::default_method(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaPluginReturnDataStringScalar(plugin_interface, default_method_.c_str(), "Plugin default method");
}

int UDAPluginBase::max_interface_version(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaPluginReturnDataIntScalar(plugin_interface, interface_version_, "Maximum Interface Version");
}

void UDAPluginBase::register_method(const std::string& name, plugin_member_type plugin_method)
{
    method_map_[name] = plugin_method;
}

void UDAPluginBase::register_function(const std::string& name, plugin_function_type plugin_function)
{
    function_map_[name] = plugin_function;
}

bool UDAPluginBase::has_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name)
{
    const char* str;
    return udaPluginFindStringArg(plugin_interface, &str, name.c_str());
}
