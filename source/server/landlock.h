#pragma once

#include <string>
#include <vector>

namespace uda::server::landlock
{

struct PathPolicy {
    bool enabled = false;
    std::vector<std::string> readonly_paths;
    std::vector<std::string> readwrite_paths;
    std::vector<std::string> execute_paths;
};

// Any configured class enables enforcement, even if all lists are empty.
PathPolicy policy_from_environment();

// Apply once, during single-threaded startup. Restrictions cannot be undone.
void enforce_path_policy(const PathPolicy& policy);

} // namespace uda::server::landlock
