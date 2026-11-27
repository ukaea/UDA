#ifndef UDA_CLIENTSERVER_UNIQUE_RESOURCE_HPP
#define UDA_CLIENTSERVER_UNIQUE_RESOURCE_HPP

#include <cstdio>
#include <memory>
#include <utility>

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

namespace uda {

struct FileCloser {
    void operator()(FILE* file) const noexcept { std::fclose(file); }
};

using UniqueFile = std::unique_ptr<FILE, FileCloser>;

#ifndef _WIN32
struct PipeCloser {
    void operator()(FILE* pipe) const noexcept { ::pclose(pipe); }
};

using UniquePipe = std::unique_ptr<FILE, PipeCloser>;
#endif

class UniqueFd {
public:
    explicit UniqueFd(int fd = -1) noexcept : fd_(fd) {}
    ~UniqueFd() { reset(); }

    UniqueFd(UniqueFd&& other) noexcept : fd_(other.release()) {}

    UniqueFd& operator=(UniqueFd&& other) noexcept
    {
        if (this != &other) {
            reset();
            fd_ = other.release();
        }
        return *this;
    }

    int get() const noexcept { return fd_; }
    int release() noexcept { return std::exchange(fd_, -1); }

    void reset() noexcept
    {
        if (fd_ >= 0) {
#ifdef _WIN32
            ::_close(release());
#else
            ::close(release());
#endif
        }
    }

private:
    int fd_;
};

} // namespace uda

#endif // UDA_CLIENTSERVER_UNIQUE_RESOURCE_HPP
