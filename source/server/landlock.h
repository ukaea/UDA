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

// Reads UDA_LANDLOCK (ON/OFF) and the UDA_LANDLOCK_*_PATHS lists. When UDA_LANDLOCK is unset, enforcement
// follows enabled_by_default (true in Landlock builds), and setting any path list also requests it.
// Enforcement fails closed: unset or empty lists grant nothing. UDA_LANDLOCK=OFF disables it and ignores the lists.
PathPolicy policy_from_environment(bool enabled_by_default);

// Apply once, during single-threaded startup. Restrictions cannot be undone.
void enforce_path_policy(const PathPolicy& policy);

} // namespace uda::server::landlock
