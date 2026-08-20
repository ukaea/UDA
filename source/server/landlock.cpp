#include "landlock.h"

#include <fcntl.h>
#include <linux/landlock.h>
#include <cstdio>
#include <cstdlib>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>

static void throw_error(const char* msg) {
    const int err = errno;
    const char* errmsg = strerror(err);
    throw std::runtime_error{std::string{msg} + ": " + errmsg};
}

static int landlock_create_ruleset_wrapper(
    const landlock_ruleset_attr* attr,
    const size_t size,
    const __u32 flags) {
    const int fd = syscall(SYS_landlock_create_ruleset,
                           attr,
                           size,
                           flags);
    if (fd < 0) {
        throw_error("landlock_create_ruleset");
    }
    return fd;
}

static void landlock_add_rule_wrapper(
    const int ruleset_fd,
    const landlock_rule_type type,
    const void* attr,
    const __u32 flags) {
    const int rc = syscall(SYS_landlock_add_rule,
                           ruleset_fd,
                           type,
                           attr,
                           flags);
    if (rc < 0) {
        throw_error("landlock_add_rule");
    }
}

static void landlock_restrict_self_wrapper(
    const int ruleset_fd,
    const __u32 flags) {
    const int rc = syscall(SYS_landlock_restrict_self,
                           ruleset_fd,
                           flags);
    if (rc < 0) {
        throw_error("landlock_add_rule");
    }
}

static int init_ruleset() {
    constexpr landlock_ruleset_attr ruleset = {
        .handled_access_fs =
        LANDLOCK_ACCESS_FS_EXECUTE |
        LANDLOCK_ACCESS_FS_READ_FILE |
        LANDLOCK_ACCESS_FS_READ_DIR,
    };

    return landlock_create_ruleset_wrapper(
        &ruleset,
        sizeof(ruleset),
        0);
}

static void add_path(int ruleset_fd, const char* path) {
    const int dir_fd = open(path, O_PATH | O_CLOEXEC);

    if (dir_fd < 0) {
        throw_error(path);
    }

    const landlock_path_beneath_attr rule = {
        .allowed_access =
        LANDLOCK_ACCESS_FS_EXECUTE |
        LANDLOCK_ACCESS_FS_READ_FILE |
        LANDLOCK_ACCESS_FS_READ_DIR,
        .parent_fd = dir_fd,
    };

    landlock_add_rule_wrapper(
        ruleset_fd,
        LANDLOCK_RULE_PATH_BENEATH,
        &rule,
        0);

    close(dir_fd);
}

static void finalise_ruleset(int ruleset_fd) {
    /*
     * Required for unprivileged restriction.
     */

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        throw_error("PR_SET_NO_NEW_PRIVS");
        return;
    }

    landlock_restrict_self_wrapper(
        ruleset_fd,
        0);

    close(ruleset_fd);
}

void uda::server::landlock::add_paths(std::vector<std::string>& paths) {
    if (paths.empty()) {
        return;
    }

    int ruleset_fd = init_ruleset();

    for (auto& path: paths) {
        add_path(ruleset_fd, path.c_str());
    }

    finalise_ruleset(ruleset_fd);
}
