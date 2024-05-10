#pragma once

#include <string>
#include <memory>
#include <uda/plugins.h>

namespace uda::client_server {

inline int noop(void*) { return 0; }

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

    PluginData() : handle{nullptr, noop} {};
};

} // namespace uda::client_server