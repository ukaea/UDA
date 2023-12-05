#pragma once

#ifndef UDA_UDA_PLUGIN_H
#define UDA_UDA_PLUGIN_H

#include <string>

#include <plugins/pluginStructs.h>
#include <boost/format.hpp>
#include "udaPlugin.h"

class UDAPluginBase;

typedef int (*plugin_function_type)(IDAM_PLUGIN_INTERFACE*);

/**
 * Abstract base class to be used to provide helper functions to make it easier to create C++ plugins.
 */
class UDAPluginBase {
public:
    typedef int (UDAPluginBase::*plugin_member_type)(IDAM_PLUGIN_INTERFACE*);
    int call(IDAM_PLUGIN_INTERFACE* plugin_interface);

protected:
    UDAPluginBase(std::string name, int version, std::string default_method, std::string help_file)
        : init_{false }
        , name_{std::move(name) }
        , version_{version }
        , interface_version_{1 }
        , default_method_{std::move(default_method) }
        , help_file_{std::move(help_file) }
        , method_map_{}
        , function_map_{}
    {
        register_method("help", &UDAPluginBase::help);
        register_method("version", &UDAPluginBase::version);
        register_method("builddate", &UDAPluginBase::build_date);
        register_method("defaultmethod", &UDAPluginBase::default_method);
        register_method("maxinterfaceversion", &UDAPluginBase::max_interface_version);
    }

    virtual int init(IDAM_PLUGIN_INTERFACE* plugin_interface) = 0;
    virtual int reset() = 0;

    void register_method(const std::string& name, plugin_member_type plugin_method);
    void register_function(const std::string& name, plugin_function_type plugin_function);

    // Helper methods
    void debug(const std::string& message);
    void error(const std::string& message);

    template <typename T>
    T required_arg(IDAM_PLUGIN_INTERFACE* plugin_interface, const std::string& name);

    // Default method implementations
    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    int do_init(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int do_reset();
    static std::string get_function(IDAM_PLUGIN_INTERFACE* plugin_interface);

    bool init_;
    std::string name_;
    int version_;
    int interface_version_;
    std::string default_method_;
    std::string help_file_;
    std::unordered_map<std::string, plugin_member_type> method_map_;
    std::unordered_map<std::string, plugin_function_type> function_map_;
};

template <>
std::string UDAPluginBase::required_arg(IDAM_PLUGIN_INTERFACE* plugin_interface, const std::string& name)
{
    const char* value;
    if (!findStringValue(&plugin_interface->request_data->nameValueList, &value, name.c_str())) {
        auto message = (boost::format("Required argument '%1%' not given") % name).str();
        error(message);
    }
    return value;
}

#endif //UDA_UDA_PLUGIN_H
