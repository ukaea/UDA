#include "utils.h"

#include <vector>
#include <logging/logging.h>
#include <clientserver/errorLog.h>

#include "udaPlugin.h"

#if defined __has_include
#  if !__has_include(<filesystem>)
#    include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#  else
#    include <filesystem>
namespace filesystem = std::filesystem;
#  endif
#else
#  include <filesystem>
namespace filesystem = std::filesystem;
#endif

#include <boost/algorithm/string.hpp>
#include <memory>
#include <string>

namespace {

bool starts_with(const std::string& string, const std::string& search_string) {
    if (string.size() < search_string.size()) {
        return false;
    }
    const auto compare_string = string.substr(0, search_string.size());
    return compare_string == search_string;
}

}

// Check if the canonical form of expanded_path starts with one of the pre-approved paths in UDA_ALLOWED_PATHS
// (';' separated). An empty or unset list rejects every path. Diagnostics go to the given interface's host log
// and the outcome is recorded on its error stack.
int udaCheckAllowedPath(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* expanded_path)
{
    if (expanded_path == nullptr) {
        UDA_PLUGIN_RAISE_ERROR(plugin_interface, "Provided File Path Not Found!");
    }

    std::string full_path;
    try {
        full_path = filesystem::canonical(expanded_path).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", expanded_path,
                       e.what());
        UDA_PLUGIN_RAISE_ERROR(plugin_interface, "Provided File Path Not Found!");
    }
    const char* env_str = std::getenv("UDA_ALLOWED_PATHS");

    std::vector<std::string> allowed_paths;
    if (env_str != nullptr && env_str[0] != '\0') {
        // Checking if environment variable exists before using it
        boost::split(allowed_paths, env_str, boost::is_any_of(";"));
    } else {
        UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_WARN, "UDA_ALLOWED_PATHS not found or empty, rejecting all paths\n");
    }

    bool good_path = false;
    for (const auto& allowed_path: allowed_paths) {
        if (starts_with(full_path, allowed_path)) {
            good_path = true;
            break;
        }
    }
    if (!good_path) {
        std::string error_msg("Bad Path Provided " + std::string(expanded_path));
        UDA_PLUGIN_RAISE_ERROR(plugin_interface, error_msg.c_str());
    }
    return 0;
}

// Legacy form, kept so existing plugin binaries still load. libuda_plugins has no error stack of its own, so the
// detailed records are dropped: a failure still returns its code and the server reports it with its generic
// fallback message. Use udaCheckAllowedPath() to report the detail.
int check_allowed_path(const char* expanded_path)
{
    auto plugin_interface = std::make_unique<IDAM_PLUGIN_INTERFACE>();
    plugin_interface->interfaceVersion = 1;
    plugin_interface->error_stack.nerrors = 0;
    return udaCheckAllowedPath(plugin_interface.get(), expanded_path);
}
