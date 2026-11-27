#include "landlock.h"

#include <common/uda_env_options.hpp>

#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace
{

constexpr const char* readonly_paths_var = "UDA_LANDLOCK_READONLY_PATHS";
constexpr const char* readwrite_paths_var = "UDA_LANDLOCK_READWRITE_PATHS";
constexpr const char* execute_paths_var = "UDA_LANDLOCK_EXECUTE_PATHS";

// Empty entries (e.g. from appending ":/path" to an empty list) grant nothing and are skipped.
std::vector<std::string> read_paths(const char* name)
{
    std::vector<std::string> paths;
    const char* value = std::getenv(name);
    if (value == nullptr)
        return paths;

    std::istringstream input{value};
    std::string path;
    while (std::getline(input, path, ':')) {
        if (path.empty())
            continue;
        if (path.front() != '/') {
            throw std::runtime_error(std::string{name} + ": expected absolute paths separated by ':', got '" + path +
                                     "'");
        }
        paths.push_back(path);
    }
    return paths;
}

} // namespace

uda::server::landlock::PathPolicy uda::server::landlock::policy_from_environment(bool enabled_by_default)
{
    PathPolicy policy;
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
        policy.enabled = enabled_by_default || std::getenv(readonly_paths_var) != nullptr ||
                         std::getenv(readwrite_paths_var) != nullptr || std::getenv(execute_paths_var) != nullptr;
    }
    if (!policy.enabled)
        return policy;
    policy.readonly_paths = read_paths(readonly_paths_var);
    policy.readwrite_paths = read_paths(readwrite_paths_var);
    policy.execute_paths = read_paths(execute_paths_var);
    return policy;
}
