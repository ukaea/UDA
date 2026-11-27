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

// Reads UDA_LANDLOCK and the UDA_LANDLOCK_{READONLY,READWRITE,EXECUTE}_PATHS lists:
// - UDA_LANDLOCK takes on/off, true/false, yes/no or 1/0; any other value throws.
// - UDA_LANDLOCK off disables enforcement and ignores the lists.
// - When UDA_LANDLOCK is unset, enforcement follows enabled_by_default (true in Landlock builds),
//   and setting any path list also requests it.
// - Enforcement fails closed: unset or empty lists grant nothing.
// - Lists are ':'-separated; empty entries are skipped and relative ones throw.
PathPolicy policy_from_environment(bool enabled_by_default);

// Apply once, during single-threaded startup. Restrictions cannot be undone.
void enforce_path_policy(const PathPolicy& policy);

} // namespace uda::server::landlock
