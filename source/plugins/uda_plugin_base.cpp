#include "uda_plugin_base.hpp"
#include "udaPlugin.h"
#include "clientserver/stringUtils.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

int UDAPluginBase::call(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    try {
        if (plugin_interface->interfaceVersion > m_interface_version) {
            RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        }

        plugin_interface->pluginVersion = m_version;

        do_reset();
        do_init(plugin_interface);

        //----------------------------------------------------------------------------------------
        // Plugin Functions
        //----------------------------------------------------------------------------------------

        std::string function = get_function(plugin_interface);

        if (m_function_map.find(function) != m_function_map.end()) {
            return m_function_map.at(function)(plugin_interface);
        } else if (m_method_map.find(function) != m_method_map.end()) {
            auto fn = m_method_map.at(function);
            return (this->*fn)(plugin_interface);
        } else {
            UDA_LOG(UDA_LOG_ERROR, "Unknown function requested %s\n", function.c_str());
            addIdamError(UDA_CODE_ERROR_TYPE, "UDAPluginBase::call", 999, "Unknown function requested");
            concatUdaError(&plugin_interface->error_stack);
            return 999;
        }
    } catch (std::exception& ex) {
        UDA_LOG(UDA_LOG_ERROR, "Exception: %s\n", ex.what());
        addIdamError(UDA_CODE_ERROR_TYPE, "UDAPluginBase::call", 999, ex.what());
        concatUdaError(&plugin_interface->error_stack);
        return 999;
    }
}

std::string UDAPluginBase::get_function(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    REQUEST_DATA* request = plugin_interface->request_data;
    return boost::to_lower_copy<std::string>(request->function);
}

int UDAPluginBase::do_init(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    std::string function = get_function(plugin_interface);
    if (!m_init || (function == "init" || function == "initialise")) {
        int rc = init(plugin_interface);
        if (rc == 0) {
            m_init = true;
        }
        return rc;
    }
    return 0;
}

int UDAPluginBase::do_reset() {
    if (!m_init) {
        // Not previously initialised: Nothing to do!
        return 0;
    }

    reset();
    m_init = false;

    return 0;
}

int UDAPluginBase::help(IDAM_PLUGIN_INTERFACE *plugin_interface) {
    std::string desc = m_name + ": help = description of this plugin";

    if (m_help_file.empty()) {
        const char* help = "No help available";
        return setReturnDataString(plugin_interface->data_block, help, desc.c_str());
    }

    auto path = boost::filesystem::path(m_help_file);
    if (!boost::filesystem::exists(path)) {
        auto help = (boost::format("help file %1% does not exist") % path).str();
        return setReturnDataString(plugin_interface->data_block, help.c_str(), desc.c_str());
    }

    std::ifstream help_file{ path.string() };
    std::stringstream buffer;
    buffer << help_file.rdbuf();
    auto help = buffer.str();

    return setReturnDataString(plugin_interface->data_block, help.c_str(), desc.c_str());
}

int UDAPluginBase::version(IDAM_PLUGIN_INTERFACE *plugin_interface) {
    return setReturnDataIntScalar(plugin_interface->data_block, m_version, "Plugin version number");
}

int UDAPluginBase::build_date(IDAM_PLUGIN_INTERFACE *plugin_interface) {
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

int UDAPluginBase::default_method(IDAM_PLUGIN_INTERFACE *plugin_interface) {
    return setReturnDataString(plugin_interface->data_block, m_default_method.c_str(), "Plugin default method");
}

int UDAPluginBase::max_interface_version(IDAM_PLUGIN_INTERFACE *plugin_interface) {
    return setReturnDataIntScalar(plugin_interface->data_block, m_interface_version, "Maximum Interface Version");
}

void UDAPluginBase::register_method(const std::string &name, plugin_member_type plugin_method) {
    m_method_map[name] = plugin_method;
}

void UDAPluginBase::register_function(const std::string &name, plugin_function_type plugin_function) {
    m_function_map[name] = plugin_function;
}

void UDAPluginBase::debug(const std::string& message) {
    UDA_LOG(UDA_LOG_DEBUG, "%s", message.c_str());
}

void UDAPluginBase::error(const std::string &message) {
    UDA_LOG(UDA_LOG_ERROR, "%s", message.c_str());
    throw std::runtime_error{ message.c_str() };
}