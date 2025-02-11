#include <boost/algorithm/string.hpp>
#include <dlfcn.h>
#include <string>
#include <filesystem>
#include <memory>

#include "plugins.hpp"

#include "clientserver/error_log.h"
#include "logging/logging.h"
#include "config/config.h"
#include "uda/plugins.h"

// constexpr int RequestReadStart = 1000;

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::config;

using namespace std::string_literals;

void uda::server::Plugins::init()
{
    if (_initialised) {
        return;
    }

    discover_plugins();

    UDA_LOG(UDA_LOG_INFO, "List of Plugins available")
    int i = 0;
    for (const auto& plugin : _plugins) {
        UDA_LOG(UDA_LOG_INFO, "[{}] {}", i, plugin.name)
        ++i;
    }

    _initialised = true;
}

void uda::server::Plugins::load_plugin(const std::filesystem::path& library)
{
    UDA_LOG(UDA_LOG_DEBUG, "Loading library {}", library.string())

    constexpr const char* info_function_name = QUOTE(UDA_PLUGIN_INFO_FUNCTION_NAME);

    void* handle = dlopen(library.c_str(), RTLD_LOCAL | RTLD_LAZY);
    if (handle == nullptr) {
        const char* err_str = dlerror();
        UDA_LOG(UDA_LOG_ERROR, "Cannot open the target shared library {}: {}", library.string(), err_str)
        return;
    }

    using info_function_t = UdaPluginInfo(*)();
    auto info_function = reinterpret_cast<info_function_t>(dlsym(handle, info_function_name));

    const char* err_str = dlerror();

    PluginData plugin = {};

    if (err_str == nullptr) {
        auto plugin_info = info_function();

        auto entry_func = reinterpret_cast<UDA_PLUGIN_ENTRY_FUNC>(dlsym(handle, plugin_info.entry_function));

        err_str = dlerror();
        if (err_str != nullptr) {
            dlclose(handle);
            UDA_LOG(UDA_LOG_ERROR, "Failed to find entry function {} from library {}: {}", plugin_info.entry_function, library.string(), err_str)
            return;
        }

        plugin.name = plugin_info.name;
        boost::to_lower(plugin.name);

        std::unique_ptr<void, DLCloseDeleter> ptr{handle};

        plugin.handle = std::move(ptr);
        plugin.library_name = library;
        plugin.entry_func_name = plugin_info.entry_function;
        plugin.entry_func = entry_func;
        plugin.type = plugin_info.type;
        switch (plugin_info.type) {
            case UDA_PLUGIN_CLASS_FILE:
                plugin.default_method = plugin_info.default_method;
                plugin.extension = plugin_info.extension;
                break;
            default:
                break;
        }
        plugin.description = plugin_info.description;
        plugin.is_private = plugin_info.is_private;
    } else {
        dlclose(handle);
        UDA_LOG(UDA_LOG_ERROR, "Failed to find plugin info function {} from library {}: {}", info_function_name, library.string(), err_str)
        return;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Plugin {} successfully loaded", plugin.name)
    _plugins.emplace_back(std::move(plugin));
}

void uda::server::Plugins::discover_plugins_in_directory(const std::filesystem::path& directory)
{
    for (const auto& entry : std::filesystem::directory_iterator{directory}) {
        if (entry.is_regular_file() && !std::filesystem::is_symlink(entry)) {
            if (entry.path().extension() == SHARED_LIBRARY_SUFFIX) {
                load_plugin(entry.path());
            }
        }
    }
}

void uda::server::Plugins::discover_plugins()
{
    auto directories_string = _config.get("plugins.directories").as_or_default(""s);
    std::vector<std::filesystem::path> directories;
    boost::split(directories, directories_string, boost::is_any_of(";"), boost::token_compress_on);

    for (const auto& directory : directories) {
        auto path = std::filesystem::directory_entry{ directory };
        if (!path.exists()) {
            UDA_LOG(UDA_LOG_ERROR, "plugin directory {} does not exist", directory.string())
            continue;
        }
        if (!path.is_directory()) {
            UDA_LOG(UDA_LOG_ERROR, "plugin directory {} is not a directory", directory.string())
            continue;
        }
        discover_plugins_in_directory(directory);
    }
}

void uda::server::Plugins::close()
{
    _plugins.clear();
    _initialised = false;
}

std::pair<size_t, boost::optional<const PluginData&>> uda::server::Plugins::find_by_name(std::string name) const
{
    boost::to_lower(name);
    size_t i = 0;
    for (const auto& plugin : _plugins) {
        if (plugin.name == name) {
            return std::make_pair(i, boost::optional<const PluginData&>{plugin});
        }
        ++i;
    }
    return std::make_pair(0, boost::optional<const PluginData&>{});
}

[[nodiscard]] boost::optional<const PluginData&> uda::server::Plugins::find_by_id(const size_t id) const
{
    if (_plugins.empty() || id > _plugins.size() - 1) {
        return {};
    }
    return _plugins[id];
}
