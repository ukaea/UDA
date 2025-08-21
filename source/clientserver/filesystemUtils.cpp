#if defined __has_include
#  if !__has_include(<filesystem>)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#  else
#include <filesystem>
namespace filesystem = std::filesystem;
#  endif
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif

#include <boost/algorithm/string.hpp>
#include <string>

// Check if path starts with pre-approved file path
// Raises Plugin Error if not
int check_allowed_path(const char* expandedPath) {
    std::string full_path;
    try { 
        full_path = filesystem::canonical(expandedPath).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_LOG(UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", full_path.c_str(), e.what());
        RAISE_PLUGIN_ERROR("Provided File Path Not Found!\n");
        return 1;
    }
    const char* env_str = std::getenv("UDA_BYTES_PLUGIN_ALLOWED_PATHS");

    std::vector<std::string> allowed_paths;
    if (env_str && !std::string(env_str).empty()) {
        // gotta check if environment variable exists before using it
        boost::split(allowed_paths, env_str, boost::is_any_of(";"));
    } else if (env_str && std::string(env_str).empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_BYTES_PLUGIN_ALLOWED_PATHS is set to an empty string, nothing all paths will be rejected (Env set in bytesPlugin.cfg.in)\n");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_BYTES_PLUGIN_ALLOWED_PATHS is was not found, rejecting all paths (Env set in bytesPlugin.cfg.in)\n");
    }

    bool good_path = false;
    for (const auto& allowed_path : allowed_paths) {
        if (full_path.rfind(allowed_path, 0) != std::string::npos) {
            good_path = true;
            break;
        }
    }
    if (!good_path) {
        UDA_LOG(UDA_LOG_ERROR, "Bad Path Provided %s\n", expandedPath);
        RAISE_PLUGIN_ERROR("Bad File Path Provided\n");
        return 1;
    }
    return 0;
}