#ifndef UDA_PLUGINS_UTILS_H
#define UDA_PLUGINS_UTILS_H

#include <clientserver/export.h>
#include <clientserver/udaDiagnostics.h>
#include <plugins/pluginStructs.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Downloadable paths come from UDA_DOWNLOADABLE_PATHS (':'-separated, ';' on Windows), or the legacy ';'-separated
// UDA_ALLOWED_PATHS. Unset or empty rejects every path. Errors are recorded on, and diagnostics logged through, the
// given plugin interface: "Bad Path Provided <path>" for a rejected path, "Provided File Path Not Found!" for a path
// that does not exist.

/**
 * Returns 0 if clients may download expanded_path, otherwise records an error on plugin_interface and returns it.
 * This checks the name only: if the file is then opened by name, a symlink swapped in between the check and the
 * open can redirect it. Use udaOpenDownloadableFile() to read the file.
 */
LIBRARY_API int udaCheckAllowedPath(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* expanded_path);

/**
 * Opens a downloadable regular file for binary reading. The check applies to the file actually opened (on Linux and
 * macOS), so a symlink swapped in after the check cannot redirect the read. On success returns 0 and sets *file,
 * which the caller closes; otherwise records an error on plugin_interface, returns it and sets *file to NULL.
 */
LIBRARY_API int udaOpenDownloadableFile(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* expanded_path,
                                        FILE** file);

/**
 * Legacy forms of udaCheckAllowedPath() and udaOpenDownloadableFile(), kept for existing plugin binaries. They return
 * the same codes but their error details and log messages are dropped (libuda_plugins has no error stack or logger
 * of its own), so the client only sees the server's generic "Request failed with error code N" message.
 */
LIBRARY_API int check_allowed_path(const char* expanded_path);
LIBRARY_API int open_downloadable_file(const char* expanded_path, FILE** file);

#ifdef __cplusplus
}

#include <string>

namespace uda {
namespace plugins {

// Separates UDA_DOWNLOADABLE_PATHS entries: ':' as in PATH and the Landlock lists, or ';' on Windows, where ':'
// appears in drive letters. The legacy UDA_ALLOWED_PATHS always uses ';'.
#ifdef _WIN32
constexpr char path_list_separator = ';';
#else
constexpr char path_list_separator = ':';
#endif

// True if canonical_path is one of the entries in path_list or lies beneath one (whole path components only).
// Entries are trimmed and resolved with filesystem::canonical; empty, relative and nonexistent entries are skipped,
// with a warning logged through diag (if given).
LIBRARY_API bool path_in_list(const std::string& canonical_path, const std::string& path_list,
                              char separator = path_list_separator, const UDA_DIAGNOSTICS* diag = nullptr);

} // namespace plugins
} // namespace uda
#endif

#endif // UDA_PLUGINS_UTILS_H
