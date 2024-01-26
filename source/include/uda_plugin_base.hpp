#pragma once

#ifndef UDA_UDA_PLUGIN_H
#define UDA_UDA_PLUGIN_H

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <fmt/format.h>

#include "udaPlugin.h"

class UDAPluginBase;

typedef int (*plugin_function_type)(UDA_PLUGIN_INTERFACE*);

/**
 * Abstract base class to be used to provide helper functions to make it easier to create C++ plugins.
 */
class UDAPluginBase {
public:
    typedef int (UDAPluginBase::*plugin_member_type)(UDA_PLUGIN_INTERFACE*);
    LIBRARY_API int call(UDA_PLUGIN_INTERFACE* plugin_interface);

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

    virtual void init(UDA_PLUGIN_INTERFACE* plugin_interface) = 0;
    virtual void reset() = 0;

    LIBRARY_API void register_method(const std::string& name, plugin_member_type plugin_method);
    LIBRARY_API void register_function(const std::string& name, plugin_function_type plugin_function);

    // Helper methods
    template <typename... Args>
    void debug(UdaPluginInterface* plugin_interface, const std::string& message, Args... args)
    {
        auto msg = fmt::format(message, args...);
        udaPluginLog(plugin_interface, "%s", msg.c_str());
    }

    template <typename... Args>
    void error(UdaPluginInterface* plugin_interface, const std::string& message, Args... args)
    {
        auto msg = fmt::format(message, args...);
        udaPluginLog(plugin_interface, "%s", msg.c_str());
        throw std::runtime_error{ msg.c_str() };
    }

    LIBRARY_API bool has_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name);

    template <typename T>
    boost::optional<T> find_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name, bool required=false)
    {
        const char* str;
        bool found = findStringValue(plugin_interface, &str, name.c_str());
        if (found) {
            std::stringstream ss(str);
            T value;
            ss >> value;
            return value;
        } else if (required) {
            error(plugin_interface, "Required argument '{}' not given", name);
        }
        return {};
    }

    template <typename T>
    T find_required_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name)
    {
        auto arg = find_arg<T>(plugin_interface, name, true);
        return *arg;
    }

    template <typename T>
    boost::optional<std::vector<T>> find_array_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name, bool required=false)
    {
        const char* str;
        bool found = findStringValue(plugin_interface, &str, name.c_str());
        if (found) {
            std::vector<std::string> tokens;
            boost::split(tokens, str, boost::is_any_of(";"));
            std::vector<T> values;
            for (const auto& token : tokens) {
                std::stringstream ss{token};
                T n;
                ss >> n;
                values.push_back(n);
            }
        } else if (required) {
            error(plugin_interface, "Required argument '{}' not given", name);
        }
        return {};
    }

    template <typename T>
    std::vector<T> find_required_array_arg(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& name)
    {
        auto arg = find_array_arg<T>(plugin_interface, name, true);
        return *arg;
    }

    // Default method implementations
    LIBRARY_API int help(UDA_PLUGIN_INTERFACE* plugin_interface);
    LIBRARY_API int version(UDA_PLUGIN_INTERFACE* plugin_interface);
    LIBRARY_API int build_date(UDA_PLUGIN_INTERFACE* plugin_interface);
    LIBRARY_API int default_method(UDA_PLUGIN_INTERFACE* plugin_interface);
    LIBRARY_API int max_interface_version(UDA_PLUGIN_INTERFACE* plugin_interface);

private:
    void do_init(UDA_PLUGIN_INTERFACE* plugin_interface);
    void do_reset();
    static std::string get_function(UDA_PLUGIN_INTERFACE* plugin_interface);

    bool init_;
    std::string name_;
    int version_;
    int interface_version_;
    std::string default_method_;
    std::string help_file_;
    std::unordered_map<std::string, plugin_member_type> method_map_;
    std::unordered_map<std::string, plugin_function_type> function_map_;
};

#endif //UDA_UDA_PLUGIN_H
