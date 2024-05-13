#pragma once

#include <string>
#include <memory>
#include <uda/plugins.h>
#include <dlfcn.h>

namespace uda::client_server {

inline int dl_close(void* ptr) { return (ptr != nullptr) ? dlclose(ptr) : 0; }

struct PluginData {
    std::string name;
    std::string version;
    std::string library_name;
    std::string entry_func_name;
    std::string default_method;
    std::string extension;
    std::string description;
    std::string example;
    UdaPluginClass type;
    UdaPluginCacheMode cache_mode;
    bool is_private;
    int interface_version;
    std::unique_ptr<void, int (*)(void*)> handle;
    UDA_PLUGIN_ENTRY_FUNC entry_func;

    PluginData() : handle{nullptr, dl_close} {};
    explicit PluginData(void* _handle) : handle{_handle, dl_close} {};
    PluginData(const PluginData&) = delete;
    PluginData& operator=(const PluginData&) = delete;
    PluginData(PluginData&&) = default;
    PluginData& operator=(PluginData&&) = default;
};

} // namespace uda::client_server