#include "pluginStreamRedirect.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <clientserver/errorLog.h>
#include <clientserver/mkstemp.h>

#include "serverPlugin.h"

#ifdef _WIN32
#  ifdef __GNUC__
#    include <unistd.h>
#  endif
#  define dup _dup
#  define dup2 _dup2
#  define fdopen _fdopen
#  define fileno _fileno
#endif

namespace {

// Redirecting stdout and stderr changes process-wide state.
uda::PluginStreamRedirect* active_redirect = nullptr;

uda::PluginStreamRedirect& plugin_redirect()
{
    if (active_redirect != nullptr) {
        return *active_redirect;
    }
    // Compatibility for callers of the installed server API outside udaServer().
    static uda::PluginStreamRedirect standalone_redirect;
    return standalone_redirect;
}

int replace_fd(int from, int to) noexcept
{
    int rc;
    do {
        rc = ::dup2(from, to);
    } while (rc < 0 && errno == EINTR);
    return rc < 0 ? errno : 0;
}

} // namespace

namespace uda {

PluginStreamRedirect::~PluginStreamRedirect()
{
    close();
    if (active_redirect == this) {
        active_redirect = previous_;
    }
}

void PluginStreamRedirect::activate() noexcept
{
    previous_ = active_redirect;
    active_redirect = this;
}

bool PluginStreamRedirect::is_capturing() const noexcept
{
    return saved_stdout_.get() >= 0 && saved_stderr_.get() >= 0;
}

int PluginStreamRedirect::open()
{
    if (file_) {
        return 0;
    }
    if (!temporary_path_.empty()) {
        // Retry removal if a prior close could not remove the temporary file.
        const int error = close();
        if (error != 0) return error;
    }

    const char* redirect_file = std::getenv("UDA_PLUGIN_REDIRECT_FILE");
    if (redirect_file != nullptr) {
        file_.reset(std::fopen(redirect_file, "w"));
    } else {
        const char* redirect_dir = std::getenv("UDA_PLUGIN_REDIRECT_DIR");
        std::string path = redirect_dir != nullptr ? redirect_dir : "/tmp";
        path += "/idamPLUGINXXXXXX";
        UniqueFd fd{::mkstemp(path.data())};
        if (fd.get() < 0) {
            const int error = errno != 0 ? errno : 994;
            UDA_THROW_ERROR(error, "Unable to Obtain a Temporary File Name");
        }

        temporary_path_.swap(path);
        file_.reset(::fdopen(fd.get(), "a"));
        if (file_) {
            fd.release(); // fdopen takes ownership on success.
        } else {
            fd.reset();
            close();
        }
    }

    if (!file_) {
        UDA_THROW_ERROR(999, "Unable to Trap Plugin Error Messages.");
    }
    return 0;
}

int PluginStreamRedirect::redirect()
{
    if (is_capturing()) {
        return 0;
    }
    int error = restore();
    if (error != 0) return error;

    const int stdout_fd = ::fileno(stdout);
    const int stderr_fd = ::fileno(stderr);
    UniqueFd original_stdout{::dup(stdout_fd)};
    if (original_stdout.get() < 0) return errno;
    // dup can reuse stderr's number if the original stderr was closed.
    if (original_stdout.get() == stderr_fd) return EBADF;
    UniqueFd original_stderr{::dup(stderr_fd)};
    if (original_stderr.get() < 0) return errno;

    error = open();
    if (error != 0) return error;

    std::fflush(stdout);
    std::fflush(stderr);
    error = replace_fd(::fileno(file_.get()), stdout_fd);
    if (error != 0) return error;
    saved_stdout_ = std::move(original_stdout);

    error = replace_fd(::fileno(file_.get()), stderr_fd);
    if (error != 0) {
        restore();
        return error;
    }
    saved_stderr_ = std::move(original_stderr);
    return 0;
}

int PluginStreamRedirect::restore() noexcept
{
    int stdout_error = 0;
    int stderr_error = 0;
    if (saved_stdout_.get() >= 0) {
        std::fflush(stdout);
        stdout_error = replace_fd(saved_stdout_.get(), ::fileno(stdout));
        if (stdout_error == 0) saved_stdout_.reset();
    }
    if (saved_stderr_.get() >= 0) {
        std::fflush(stderr);
        stderr_error = replace_fd(saved_stderr_.get(), ::fileno(stderr));
        if (stderr_error == 0) saved_stderr_.reset();
    }
    return stdout_error != 0 ? stdout_error : stderr_error;
}

int PluginStreamRedirect::close() noexcept
{
    const int restore_error = restore();
    int file_error = 0;
    int remove_error = 0;
    if (file_ && std::fclose(file_.release()) != 0) {
        file_error = errno;
    }
    if (!temporary_path_.empty()) {
        if (std::remove(temporary_path_.c_str()) == 0 || errno == ENOENT) {
            temporary_path_.clear();
        } else {
            remove_error = errno;
        }
    }
    if (restore_error != 0) return restore_error;
    return file_error != 0 ? file_error : remove_error;
}

PluginOutputCapture::PluginOutputCapture()
{
    auto& redirect = plugin_redirect();
    if (!redirect.is_capturing()) {
        owner_ = &redirect;
        status_ = redirect.redirect();
    }
}

PluginOutputCapture::~PluginOutputCapture()
{
    restore();
}

int PluginOutputCapture::restore() noexcept
{
    if (owner_ == nullptr) {
        return 0;
    }
    const int error = owner_->restore();
    if (error == 0) owner_ = nullptr;
    return error;
}

} // namespace uda

int udaServerRedirectStdStreams(bool reset, bool cleanup)
{
    auto& redirect = plugin_redirect();
    if (!cleanup && !reset) {
        return redirect.redirect();
    }

    const int error = cleanup ? redirect.close() : redirect.restore();
    if (error != 0) {
        UDA_THROW_ERROR(error, std::strerror(error));
    }
    return 0;
}
