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

// Check if path starts with pre-approved file path
// Raises Plugin Error if not
int check_allowed_path(const char* expanded_path) {
    std::string full_path;
    try {
        full_path = filesystem::canonical(expanded_path).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_LOG(UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", full_path.c_str(), e.what());
        RAISE_PLUGIN_ERROR("Provided File Path Not Found!");
    }
    const char* env_str = std::getenv("UDA_ALLOWED_PATHS");

    std::vector<std::string> allowed_paths;
    if (env_str != nullptr && env_str[0] != '\0') {
        // Checking if environment variable exists before using it
        boost::split(allowed_paths, env_str, boost::is_any_of(";"));
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_ALLOWED_PATHS not found or empty, rejecting all paths\n");
    }

    bool good_path = false;
    for (const auto& allowed_path: allowed_paths) {
        if (starts_with(full_path, allowed_path)) {
            good_path = true;
            break;
        }
    }
    if (!good_path) {
        UDA_LOG(UDA_LOG_ERROR, "Bad Path Provided %s\n", expanded_path);
        std::string error_msg("Bad Path Provided " + std::string(expanded_path));
        RAISE_PLUGIN_ERROR(error_msg.c_str());
    }
    return 0;
}