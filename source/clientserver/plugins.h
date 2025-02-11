#pragma once

#include <string>
#include <memory>
#include <uda/plugins.h>
#include <dlfcn.h>

#include "logging/logging.h"

namespace uda::client_server {

struct DLCloseDeleter {
    void operator()(void* handle) const { if (handle != nullptr) { dlclose(handle); } }
};

struct PluginData {
    std::string name;
    std::string version;
    std::string library_name;
    std::string entry_func_name;
    std::string default_method;
    std::string extension;
    std::string description;
    std::string example;
    UdaPluginClass type = UDA_PLUGIN_CLASS_UNKNOWN;
    UdaPluginCacheMode cache_mode = UDA_PLUGIN_CACHE_MODE_NONE;
    bool is_private = false;
    int interface_version = 0;
    std::unique_ptr<void, DLCloseDeleter> handle;
    UDA_PLUGIN_ENTRY_FUNC entry_func = nullptr;

    PluginData() = default;
    ~PluginData() = default;
    explicit PluginData(void* _handle) : handle{_handle} {};
    PluginData(const PluginData&) = delete;
    PluginData& operator=(const PluginData&) = delete;
    PluginData(PluginData&&) = default;
    PluginData& operator=(PluginData&&) = default;
};

} // namespace uda::client_server