#include "landlock.h"

#include <cstdlib>
#include <stdexcept>

namespace
{

void read_paths(const char* name, bool& enabled, std::vector<std::string>& paths)
{
    const char* value = std::getenv(name);
    if (value == nullptr)
        return;
    enabled = true;
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

uda::server::landlock::PathPolicy uda::server::landlock::policy_from_environment()
{
    if (std::getenv("UDA_ENABLE_PATHS") != nullptr) {
        throw std::runtime_error("UDA_ENABLE_PATHS has been replaced by UDA_LANDLOCK_READONLY_PATHS, "
                                 "UDA_LANDLOCK_READWRITE_PATHS and UDA_LANDLOCK_EXECUTE_PATHS");
    }
    PathPolicy policy;
    read_paths("UDA_LANDLOCK_READONLY_PATHS", policy.enabled, policy.readonly_paths);
    read_paths("UDA_LANDLOCK_READWRITE_PATHS", policy.enabled, policy.readwrite_paths);
    read_paths("UDA_LANDLOCK_EXECUTE_PATHS", policy.enabled, policy.execute_paths);
    return policy;
}
