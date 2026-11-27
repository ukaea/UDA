#include "landlock.h"

#include <common/uda_env_options.hpp>

#include <cstdlib>
#include <stdexcept>

namespace
{

// Empty entries (e.g. from appending ":/path" to an empty list) grant nothing and are skipped.
void read_paths(const char* name, std::vector<std::string>& paths)
{
    const char* value = std::getenv(name);
    if (value == nullptr)
        return;
    const std::string input{value};

    size_t start = 0;
    while (start <= input.size()) {
        auto end = input.find(':', start);
        if (end == std::string::npos)
            end = input.size();
        const auto path = input.substr(start, end - start);
        if (!path.empty()) {
            if (path.front() != '/') {
                throw std::runtime_error(std::string{name} + ": expected absolute paths separated by ':', got '" +
                                         path + "'");
            }
            paths.push_back(path);
        }
        start = end + 1;
    }
}

} // namespace

uda::server::landlock::PathPolicy uda::server::landlock::policy_from_environment(bool enabled_by_default)
{
    if (std::getenv("UDA_ENABLE_PATHS") != nullptr) {
        throw std::runtime_error("UDA_ENABLE_PATHS has been replaced by UDA_LANDLOCK_READONLY_PATHS, "
                                 "UDA_LANDLOCK_READWRITE_PATHS and UDA_LANDLOCK_EXECUTE_PATHS");
    }
    PathPolicy policy;
    const char* path_variables[] = {"UDA_LANDLOCK_READONLY_PATHS", "UDA_LANDLOCK_READWRITE_PATHS",
                                    "UDA_LANDLOCK_EXECUTE_PATHS"};
    if (const char* setting = std::getenv("UDA_LANDLOCK")) {
        // Same spellings as other UDA switches, but reject anything else instead of falling back to the
        // default: a typo must not leave a server that cannot enforce Landlock running unconfined.
        namespace env = uda::common::env_config;
        if (env::strings_match(setting, env::truthy_values)) {
            policy.enabled = true;
        } else if (!env::strings_match(setting, env::falsey_values)) {
            throw std::runtime_error(std::string{"UDA_LANDLOCK: expected on/off, true/false, yes/no or 1/0, got '"} +
                                     setting + "'");
        }
    } else {
        policy.enabled = enabled_by_default;
        for (const char* name : path_variables) {
            policy.enabled = policy.enabled || std::getenv(name) != nullptr;
        }
    }
    if (!policy.enabled)
        return policy;
    read_paths(path_variables[0], policy.readonly_paths);
    read_paths(path_variables[1], policy.readwrite_paths);
    read_paths(path_variables[2], policy.execute_paths);
    return policy;
}
