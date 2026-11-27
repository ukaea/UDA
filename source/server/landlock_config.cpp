#include "landlock.h"

#include <cstdlib>
#include <strings.h>
#include <stdexcept>

namespace
{

void read_paths(const char* name, std::vector<std::string>& paths)
{
    const char* value = std::getenv(name);
    if (value == nullptr)
        return;
    const std::string input{value};
    if (input.empty())
        return;

    size_t start = 0;
    while (true) {
        const auto end = input.find(':', start);
        const auto path = input.substr(start, end == std::string::npos ? end : end - start);
        if (path.empty() || path.front() != '/') {
            throw std::runtime_error(std::string{name} + ": expected nonempty absolute paths separated by ':'");
        }
        paths.push_back(path);
        if (end == std::string::npos)
            break;
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
        if (strcasecmp(setting, "ON") == 0) {
            policy.enabled = true;
        } else if (strcasecmp(setting, "OFF") != 0) {
            throw std::runtime_error(std::string{"UDA_LANDLOCK: expected ON or OFF, got '"} + setting + "'");
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
