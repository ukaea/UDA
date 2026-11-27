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
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>

#ifndef _WIN32
#  include <fcntl.h>
#  include <sys/param.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace {

// True if path is root itself or lies beneath it. Compares whole path components, so /data does not contain /data2.
bool path_is_within(const filesystem::path& path, const filesystem::path& root)
{
    auto mismatch = std::mismatch(root.begin(), root.end(), path.begin(), path.end());
    return mismatch.first == root.end();
}

struct PathList {
    std::string value;
    char separator;
};

// UDA_DOWNLOADABLE_PATHS, falling back to its legacy name UDA_ALLOWED_PATHS when unset or empty. The new name uses
// the platform's path-list separator, as the Landlock lists and PATH do; the legacy name keeps its ';'.
PathList downloadable_paths()
{
    const char* value = std::getenv("UDA_DOWNLOADABLE_PATHS");
    const char* legacy = std::getenv("UDA_ALLOWED_PATHS");
    const bool have_value = value != nullptr && value[0] != '\0';
    const bool have_legacy = legacy != nullptr && legacy[0] != '\0';
    if (have_value && have_legacy && std::strcmp(value, legacy) != 0) {
        UDA_LOG(UDA_LOG_WARN, "UDA_ALLOWED_PATHS is ignored because UDA_DOWNLOADABLE_PATHS is set\n");
    }
    if (have_value) {
        if (uda::plugins::path_list_separator != ';' && std::strchr(value, ';') != nullptr) {
            UDA_LOG(UDA_LOG_WARN, "UDA_DOWNLOADABLE_PATHS contains ';': entries are separated by '%c' "
                                  "(only the legacy UDA_ALLOWED_PATHS uses ';')\n", uda::plugins::path_list_separator);
        }
        return {value, uda::plugins::path_list_separator};
    }
    return {have_legacy ? legacy : "", ';'};
}

#if defined(__linux__) || defined(__APPLE__)
#  define UDA_HAVE_OPENED_PATH 1
// Where the file open on fd actually is, as the kernel resolved it at open: no symlinks or "..". Empty if the
// platform cannot say, in which case the caller must not trust the file.
std::string opened_path(int fd)
{
#  if defined(__linux__)
    std::error_code error;
    const auto path = filesystem::read_symlink("/proc/self/fd/" + std::to_string(fd), error);
    return error ? std::string{} : path.string();
#  else
    char buffer[MAXPATHLEN];
    return fcntl(fd, F_GETPATH, buffer) == 0 ? std::string{buffer} : std::string{};
#  endif
}
#endif

int reject_path(const char* expanded_path, const PathList& path_list)
{
    if (path_list.value.empty()) {
        UDA_LOG(UDA_LOG_WARN, "UDA_DOWNLOADABLE_PATHS (or legacy UDA_ALLOWED_PATHS) not set or empty, "
                              "rejecting all paths\n");
    }
    UDA_LOG(UDA_LOG_ERROR, "Bad Path Provided %s\n", expanded_path);
    std::string error_msg("Bad Path Provided " + std::string(expanded_path));
    RAISE_PLUGIN_ERROR(error_msg.c_str());
}

} // namespace

bool uda::plugins::path_in_list(const std::string& canonical_path, const std::string& path_list, char separator)
{
    std::vector<std::string> entries;
    boost::split(entries, path_list, [separator](char c) { return c == separator; });
    const filesystem::path path{canonical_path};
    for (auto& entry : entries) {
        boost::trim(entry);
        if (entry.empty()) {
            continue; // e.g. a trailing separator: an empty prefix would otherwise match every path
        }
        const filesystem::path entry_path{entry};
        if (!entry_path.is_absolute()) {
            UDA_LOG(UDA_LOG_WARN, "Ignoring relative downloadable path entry [%s]\n", entry.c_str());
            continue;
        }
        // Resolve symlinks and "..", as the requested path already has been, so the comparison is like for like.
        std::error_code error;
        const auto root = filesystem::canonical(entry_path, error);
        if (error) {
            UDA_LOG(UDA_LOG_WARN, "Ignoring downloadable path entry [%s]: %s\n", entry.c_str(),
                    error.message().c_str());
            continue;
        }
        if (path_is_within(path, root)) {
            return true;
        }
    }
    return false;
}

// Check that the requested path is in a client-downloadable location (UDA_DOWNLOADABLE_PATHS).
// Raises a plugin error if not. This checks the name, so prefer open_downloadable_file, which checks the opened file.
int check_allowed_path(const char* expanded_path)
{
    std::string full_path;
    try {
        full_path = filesystem::canonical(expanded_path).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_LOG(UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", expanded_path, e.what());
        RAISE_PLUGIN_ERROR("Provided File Path Not Found!");
    }

    const PathList path_list = downloadable_paths();
    if (!uda::plugins::path_in_list(full_path, path_list.value, path_list.separator)) {
        return reject_path(expanded_path, path_list);
    }
    return 0;
}

int open_downloadable_file(const char* expanded_path, FILE** file)
{
    *file = nullptr;
    const PathList path_list = downloadable_paths();

#ifdef UDA_HAVE_OPENED_PATH
    // Open first, then check where the opened file really is. Checking the name and then opening it would let a
    // symlink swapped into a downloadable directory in between redirect the read anywhere the server can read.
    // O_NONBLOCK stops a FIFO blocking the open; anything but a regular file is rejected below.
    const int fd = open(expanded_path, O_RDONLY | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        const int error = errno;
        UDA_LOG(UDA_LOG_DEBUG, "Unable to open [%s]: %s\n", expanded_path, std::strerror(error));
        if (error == ENOENT || error == ENOTDIR) {
            RAISE_PLUGIN_ERROR("Provided File Path Not Found!");
        }
        addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, error, "");
        RAISE_PLUGIN_ERROR("Unable to Open the File for Read Access");
    }

    const std::string real_path = opened_path(fd);
    if (real_path.empty() || !uda::plugins::path_in_list(real_path, path_list.value, path_list.separator)) {
        if (real_path.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Unable to determine where [%s] was opened from\n", expanded_path);
        }
        close(fd);
        return reject_path(expanded_path, path_list);
    }

    struct stat status = {};
    if (fstat(fd, &status) != 0 || !S_ISREG(status.st_mode)) {
        close(fd);
        RAISE_PLUGIN_ERROR("Provided File Path is not a Regular File");
    }
    const int flags = fcntl(fd, F_GETFL);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }

    *file = fdopen(fd, "rb");
    if (*file == nullptr) {
        addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "");
        close(fd);
        RAISE_PLUGIN_ERROR("Unable to Open the File for Read Access");
    }
    return 0;
#else
    // No way to ask where an open file is: check the resolved name and open that, not the name as given. A
    // symlink swapped into a downloadable directory between the check and the open can still redirect the read.
    std::string full_path;
    try {
        full_path = filesystem::canonical(expanded_path).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_LOG(UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", expanded_path, e.what());
        RAISE_PLUGIN_ERROR("Provided File Path Not Found!");
    }
    if (!uda::plugins::path_in_list(full_path, path_list.value, path_list.separator)) {
        return reject_path(expanded_path, path_list);
    }
    if (!filesystem::is_regular_file(full_path)) {
        RAISE_PLUGIN_ERROR("Provided File Path is not a Regular File");
    }
    *file = fopen(full_path.c_str(), "rb");
    if (*file == nullptr) {
        addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "");
        RAISE_PLUGIN_ERROR("Unable to Open the File for Read Access");
    }
    return 0;
#endif
}
