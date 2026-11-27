#include "landlock.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <linux/landlock.h>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace
{

class FileDescriptor
{
  public:
    explicit FileDescriptor(int fd) : fd_(fd) {}
    ~FileDescriptor()
    {
        if (fd_ >= 0)
            close(fd_);
    }
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    int get() const { return fd_; }

  private:
    int fd_;
};

[[noreturn]] void throw_error(const std::string& operation)
{
    const int error = errno;
    throw std::runtime_error(operation + ": " + std::strerror(error));
}

constexpr uint64_t read_access = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR;
constexpr uint64_t write_access = LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_TRUNCATE |
                                  LANDLOCK_ACCESS_FS_REMOVE_DIR | LANDLOCK_ACCESS_FS_REMOVE_FILE |
                                  LANDLOCK_ACCESS_FS_MAKE_DIR | LANDLOCK_ACCESS_FS_MAKE_REG |
                                  LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_MAKE_FIFO |
                                  LANDLOCK_ACCESS_FS_MAKE_SYM | LANDLOCK_ACCESS_FS_REFER;
constexpr uint64_t execute_access = read_access | LANDLOCK_ACCESS_FS_EXECUTE;
constexpr uint64_t handled_access = read_access | write_access | LANDLOCK_ACCESS_FS_EXECUTE |
                                    LANDLOCK_ACCESS_FS_MAKE_CHAR | LANDLOCK_ACCESS_FS_MAKE_BLOCK;
constexpr uint64_t file_access = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_WRITE_FILE |
                                 LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_TRUNCATE;

void add_path(int ruleset_fd, const std::string& path, uint64_t access)
{
    if (path.empty() || path.front() != '/') {
        throw std::runtime_error("Landlock path must be absolute: '" + path + "'");
    }
    const FileDescriptor fd{open(path.c_str(), O_PATH | O_CLOEXEC)};
    if (fd.get() < 0)
        throw_error("open Landlock path '" + path + "'");

    struct stat status{};
    if (fstat(fd.get(), &status) < 0)
        throw_error("fstat Landlock path '" + path + "'");
    landlock_path_beneath_attr rule{};
    rule.allowed_access = S_ISDIR(status.st_mode) ? access : access & file_access;
    rule.parent_fd = fd.get();
    if (syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &rule, 0) < 0) {
        throw_error("landlock_add_rule for '" + path + "'");
    }
}

} // namespace

void uda::server::landlock::enforce_path_policy(const PathPolicy& policy)
{
    if (!policy.enabled)
        return;

    const long abi = syscall(SYS_landlock_create_ruleset, nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 0)
        throw_error("Landlock unavailable (landlock_create_ruleset ABI query)");
    if (abi < 3) {
        throw std::runtime_error(
            "Landlock ABI 3 or newer is required to restrict file truncation; kernel provides ABI " +
            std::to_string(abi));
    }

    landlock_ruleset_attr attributes{};
    attributes.handled_access_fs = handled_access;
    const FileDescriptor ruleset{
        static_cast<int>(syscall(SYS_landlock_create_ruleset, &attributes, sizeof(attributes), 0))};
    if (ruleset.get() < 0)
        throw_error("landlock_create_ruleset");

    for (const auto& path : policy.readonly_paths)
        add_path(ruleset.get(), path, read_access);
    for (const auto& path : policy.readwrite_paths)
        add_path(ruleset.get(), path, read_access | write_access);
    for (const auto& path : policy.execute_paths)
        add_path(ruleset.get(), path, execute_access);

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
        throw_error("PR_SET_NO_NEW_PRIVS");
    if (syscall(SYS_landlock_restrict_self, ruleset.get(), 0) < 0)
        throw_error("landlock_restrict_self");
}
