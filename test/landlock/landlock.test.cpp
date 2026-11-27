#include <server/landlock.h>

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <link.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using uda::server::landlock::enforce_path_policy;
using uda::server::landlock::PathPolicy;
using uda::server::landlock::policy_from_environment;

static void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

static void expect_failure(const std::function<void()>& operation, const std::string& message)
{
    try {
        operation();
    } catch (const std::runtime_error& error) {
        require(std::string{error.what()}.find(message) != std::string::npos, error.what());
        return;
    }
    throw std::runtime_error("expected error containing: " + message);
}

static void check_open(const std::string& path, int flags, bool allowed)
{
    const int fd = open(path.c_str(), flags, 0600);
    const int error = errno;
    if (fd >= 0)
        close(fd);
    require(allowed ? fd >= 0 : fd < 0 && error == EACCES,
            "unexpected open result for " + path + ": " + std::strerror(error));
}

static int descriptors()
{
    DIR* directory = opendir("/proc/self/fd");
    require(directory != nullptr, "opendir /proc/self/fd");
    int count = 0;
    while (readdir(directory))
        ++count;
    closedir(directory);
    return count;
}

static void clear_environment()
{
    for (const char* key :
         {"UDA_LANDLOCK", "UDA_LANDLOCK_READONLY_PATHS", "UDA_LANDLOCK_READWRITE_PATHS", "UDA_LANDLOCK_EXECUTE_PATHS"})
        unsetenv(key);
}

static void configuration()
{
    clear_environment();
    // Landlock builds fail closed: no settings means enforcement with no grants.
    auto policy = policy_from_environment(true);
    require(policy.enabled && policy.readonly_paths.empty() && policy.readwrite_paths.empty() &&
                policy.execute_paths.empty(),
            "unset policy in a Landlock build must enforce with no grants");
    require(!policy_from_environment(false).enabled, "unset policy without Landlock support must be disabled");
    setenv("UDA_LANDLOCK_READONLY_PATHS", "", 1);
    require(policy_from_environment(false).enabled, "a configured path list must request enforcement");
    for (const char* invalid : {"relative", "/tmp:relative", " /tmp"}) {
        setenv("UDA_LANDLOCK_READONLY_PATHS", invalid, 1);
        expect_failure([] { policy_from_environment(true); }, "UDA_LANDLOCK_READONLY_PATHS");
    }
    // Empty entries grant nothing, so appending ":/path" to an empty list works.
    for (const char* sparse : {":/tmp", "/tmp:", "::/tmp::", ":"}) {
        setenv("UDA_LANDLOCK_READONLY_PATHS", sparse, 1);
        const auto expected = std::string{sparse} == ":" ? std::vector<std::string>{}
                                                         : std::vector<std::string>{"/tmp"};
        require(policy_from_environment(true).readonly_paths == expected,
                std::string{"empty entries must be skipped: "} + sparse);
    }
    setenv("UDA_LANDLOCK_READONLY_PATHS", "/tmp/a path:/usr", 1);
    setenv("UDA_LANDLOCK_READWRITE_PATHS", "/tmp/output", 1);
    setenv("UDA_LANDLOCK_EXECUTE_PATHS", "/bin/true", 1);
    policy = policy_from_environment(true);
    require(policy.readonly_paths == std::vector<std::string>{"/tmp/a path", "/usr"}, "read-only parsing");
    require(policy.readwrite_paths == std::vector<std::string>{"/tmp/output"}, "read/write parsing");
    require(policy.execute_paths == std::vector<std::string>{"/bin/true"}, "execute parsing");

    // An explicit switch overrides the default and, when OFF, the path lists.
    // Accepted spellings are shared with other UDA switches (common/uda_env_options.hpp).
    for (const char* off : {"off", "OFF", "false", "No", "0"}) {
        setenv("UDA_LANDLOCK", off, 1);
        for (bool supported : {true, false}) {
            policy = policy_from_environment(supported);
            require(!policy.enabled && policy.readonly_paths.empty(),
                    std::string{"UDA_LANDLOCK="} + off + " must disable enforcement");
        }
    }
    setenv("UDA_LANDLOCK_READONLY_PATHS", "relative", 1);
    require(!policy_from_environment(true).enabled, "UDA_LANDLOCK off must not parse path lists");
    clear_environment();
    for (const char* on : {"on", "ON", "TRUE", "yes", "1"}) {
        setenv("UDA_LANDLOCK", on, 1);
        require(policy_from_environment(false).enabled, std::string{"UDA_LANDLOCK="} + on + " must request enforcement");
    }
    // Unlike evaluate_bool_param, unrecognised values are errors rather than the default.
    for (const char* invalid : {"", "maybe", "ONN", "2", "of"}) {
        setenv("UDA_LANDLOCK", invalid, 1);
        expect_failure([] { policy_from_environment(true); }, "UDA_LANDLOCK: expected on/off");
        expect_failure([] { policy_from_environment(false); }, "UDA_LANDLOCK: expected on/off");
    }
    clear_environment();
}

static void check_exec(const std::string& path, bool allowed)
{
    const pid_t child = fork();
    require(child >= 0, "fork exec test");
    if (child == 0) {
        execl(path.c_str(), path.c_str(), static_cast<char*>(nullptr));
        _exit(errno == EACCES ? 77 : 78);
    }
    int status = 0;
    require(waitpid(child, &status, 0) == child, "wait exec test");
    require(WIFEXITED(status) && WEXITSTATUS(status) == (allowed ? 0 : 77), "unexpected exec result: " + path);
}

static std::string executable_interpreter()
{
    FILE* file = fopen("/bin/true", "rb");
    require(file != nullptr, "open ELF fixture");
    ElfW(Ehdr) header{};
    require(fread(&header, sizeof(header), 1, file) == 1, "read ELF header");
    for (int i = 0; i < header.e_phnum; ++i) {
        ElfW(Phdr) program{};
        require(fseek(file, header.e_phoff + i * header.e_phentsize, SEEK_SET) == 0, "seek ELF header");
        require(fread(&program, sizeof(program), 1, file) == 1, "read ELF program header");
        if (program.p_type == PT_INTERP) {
            std::string path(program.p_filesz, '\0');
            require(fseek(file, program.p_offset, SEEK_SET) == 0, "seek ELF interpreter");
            require(fread(path.data(), path.size(), 1, file) == 1, "read ELF interpreter");
            fclose(file);
            path.pop_back();
            return path;
        }
    }
    fclose(file);
    return {}; // A statically linked executable has no interpreter.
}

static void exercise(const std::string& name, const std::string& root)
{
    PathPolicy policy;
    policy.enabled = true;
    const auto ro = root + "/ro";
    const auto rw = root + "/rw";
    const auto outside = root + "/outside";
    if (name == "unavailable") {
        // Reproduce an unavailable syscall without depending on the host's kernel configuration.
        sock_filter filter[] = {
            BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_landlock_create_ruleset, 0, 1),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
        };
        sock_fprog program{static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0])), filter};
        require(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "seccomp no_new_privs");
        require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "install seccomp test filter");
        expect_failure([&] { enforce_path_policy(policy); }, "Landlock unavailable");
        policy.enabled = false;
        enforce_path_policy(policy); // Explicitly disabled policies do not require kernel support.
        return;
    }
    if (name == "readonly" || name == "symlink" || name == "library")
        policy.readonly_paths = {ro};
    else if (name == "readwrite")
        policy.readwrite_paths = {rw};
    else if (name == "files") {
        policy.readonly_paths = {ro + "/data"};
        policy.readwrite_paths = {rw + "/data"};
        policy.execute_paths = {root + "/exec/true"};
    } else if (name == "execute") {
        policy.readonly_paths = {ro};
        policy.readwrite_paths = {rw};
        policy.execute_paths = {root + "/exec"};
    } else if (name == "overlap") {
        policy.readonly_paths = {root};
        policy.readwrite_paths = {rw};
    } else if (name == "cleanup") {
        const int before = descriptors();
        for (int i = 0; i < 20; ++i) {
            // A successfully added rule followed by an invalid path must release all descriptors.
            policy.readonly_paths = {ro, root + "/missing"};
            expect_failure([&] { enforce_path_policy(policy); }, root + "/missing");
        }
        require(descriptors() == before, "file descriptors leaked after setup failure");
        policy.readonly_paths = {"relative"};
        expect_failure([&] { enforce_path_policy(policy); }, "absolute");
        policy.readonly_paths = {""};
        expect_failure([&] { enforce_path_policy(policy); }, "absolute");
        check_open(outside + "/data", O_RDWR, true);
        return;
    } else
        require(name == "empty", "unknown test: " + name);

    if (name == "execute" || name == "files" || name == "library") {
        // Shared libraries/interpreters need read permission, not LANDLOCK_ACCESS_FS_EXECUTE.
        for (const char* path : {"/usr", "/lib", "/lib64", "/etc/ld.so.cache"}) {
            if (fs::exists(path))
                policy.readonly_paths.emplace_back(path);
        }
    }
    if (name == "execute" || name == "files") {
        const auto interpreter = executable_interpreter();
        if (!interpreter.empty())
            policy.execute_paths.push_back(interpreter);
    }
    enforce_path_policy(policy);

    if (name == "empty") {
        check_open(ro + "/data", O_RDONLY, false);
        check_open(rw + "/data", O_WRONLY | O_TRUNC, false);
        return;
    }
    if (name == "symlink") {
        check_open(ro + "/escape", O_RDONLY, false);
        check_open(ro + "/inside", O_RDONLY, true);
        return;
    }
    if (name == "library") {
        // The same existing library outside the allowlist must remain inaccessible.
        check_open(outside + "/plugin.so", O_RDONLY, false);
        void* denied = dlopen((outside + "/plugin.so").c_str(), RTLD_NOW);
        if (denied != nullptr)
            dlclose(denied);
        require(denied == nullptr, "shared library outside the allowlist could be loaded");
        dlerror(); // Discard the expected denial before checking the permitted load.
        void* handle = dlopen((ro + "/plugin.so").c_str(), RTLD_NOW);
        if (handle == nullptr) {
            const char* error = dlerror();
            throw std::runtime_error("read-only shared library could not be loaded: " +
                                     std::string(error != nullptr ? error : "unknown loader error"));
        }
        auto function = reinterpret_cast<int (*)()>(dlsym(handle, "landlock_fixture_value"));
        require(function != nullptr && function() == 42, "shared library call failed");
        dlclose(handle);
        check_open(ro + "/plugin.so", O_WRONLY, false);
        return;
    }
    if (name == "execute") {
        check_exec(ro + "/true", false);
        check_exec(rw + "/true", false);
        check_exec(root + "/exec/true", true);
        check_open(root + "/exec/true", O_WRONLY, false);
    }
    if (name == "files") {
        check_open(ro + "/data", O_RDONLY, true);
        check_open(ro + "/data", O_WRONLY, false);
        check_open(ro + "/sibling", O_RDONLY, false);
        check_open(ro, O_RDONLY | O_DIRECTORY, false);
        check_open(rw + "/data", O_RDWR | O_TRUNC, true);
        require(truncate((rw + "/data").c_str(), 0) == 0, "file truncation should succeed");
        check_open(rw + "/new", O_CREAT | O_WRONLY, false);
        require(unlink((rw + "/data").c_str()) < 0 && errno == EACCES, "file grant must not allow unlink");
        check_exec(root + "/exec/true", true);
    }
    if (name == "readonly" || name == "overlap") {
        check_open(ro + "/data", O_RDONLY, true);
        check_open(ro, O_RDONLY | O_DIRECTORY, true);
        check_open(ro + "/data", O_WRONLY, false);
        check_open(ro + "/new", O_CREAT | O_WRONLY, false);
        require(truncate((ro + "/data").c_str(), 0) < 0 && errno == EACCES, "read-only truncate allowed");
        require(unlink((ro + "/data").c_str()) < 0 && errno == EACCES, "read-only unlink allowed");
        check_exec(ro + "/true", false);
    }
    if (name == "readwrite" || name == "overlap") {
        check_open(rw + "/data", O_RDWR | O_TRUNC, true);
        check_open(rw + "/new", O_CREAT | O_WRONLY, true);
        require(mkdir((rw + "/subdir").c_str(), 0700) == 0, "mkdir failed");
        require(rename((rw + "/new").c_str(), (rw + "/subdir/new").c_str()) == 0, "rename failed");
        require(link((rw + "/data").c_str(), (rw + "/subdir/link").c_str()) == 0, "hard link failed");
        require(unlink((rw + "/subdir/new").c_str()) == 0, "unlink failed");
        require(rename((rw + "/data").c_str(), (outside + "/moved").c_str()) < 0, "rename escaped writable root");
        require(rmdir((rw + "/empty").c_str()) == 0, "rmdir failed");
        check_exec(rw + "/true", false);
    }
    check_open(outside + "/data", O_RDONLY, name == "overlap");
    check_open(outside + "/data", O_WRONLY | O_TRUNC, false);
    check_open(outside + "/new", O_CREAT | O_WRONLY, false);
    require(unlink((outside + "/data").c_str()) < 0 && errno == EACCES, "outside unlink allowed");
    require(mkdir((outside + "/newdir").c_str(), 0700) < 0 && errno == EACCES, "outside mkdir allowed");
}

int main(int argc, char** argv)
{
    if (argc != 3)
        return 2;
    const std::string name{argv[1]};
    std::string root;
    try {
        if (name == "config") {
            configuration();
            return 0;
        }
        char pattern[] = "/tmp/uda-landlock-XXXXXX";
        const char* directory = mkdtemp(pattern);
        require(directory != nullptr, "mkdtemp failed");
        root = directory;
        for (const char* category : {"ro", "rw", "outside", "exec"}) {
            const auto path = root + "/" + category;
            fs::create_directory(path);
            for (const char* file : {"data", "sibling"}) {
                int fd = open((path + "/" + file).c_str(), O_CREAT | O_WRONLY, 0600);
                require(fd >= 0, "fixture file creation failed");
                close(fd);
            }
            fs::copy_file("/bin/true", path + "/true");
            chmod((path + "/true").c_str(), 0700);
        }
        fs::create_directory(root + "/rw/empty");
        fs::create_symlink(root + "/outside/data", root + "/ro/escape");
        fs::create_symlink(root + "/ro/data", root + "/ro/inside");
        fs::copy_file(argv[2], root + "/ro/plugin.so");
        fs::copy_file(argv[2], root + "/outside/plugin.so");
        const pid_t child = fork();
        require(child >= 0, "fork failed");
        if (child == 0) {
            try {
                exercise(name, root);
                _exit(0);
            } catch (const std::exception& error) {
                fprintf(stderr, "%s: %s\n", name.c_str(), error.what());
                _exit(1);
            }
        }
        int status = 0;
        require(waitpid(child, &status, 0) == child, "waitpid failed");
        fs::remove_all(root);
        require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child test failed: " + name);
        return 0;
    } catch (const std::exception& error) {
        fprintf(stderr, "%s\n", error.what());
        if (!root.empty())
            fs::remove_all(root);
        return 1;
    }
}
