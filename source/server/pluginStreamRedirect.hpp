#ifndef UDA_SERVER_PLUGIN_STREAM_REDIRECT_HPP
#define UDA_SERVER_PLUGIN_STREAM_REDIRECT_HPP

#include <string>

#include <clientserver/unique_resource.hpp>

namespace uda {

class PluginStreamRedirect {
public:
    PluginStreamRedirect() = default;
    ~PluginStreamRedirect();

    PluginStreamRedirect(const PluginStreamRedirect&) = delete;
    PluginStreamRedirect& operator=(const PluginStreamRedirect&) = delete;

    void activate() noexcept;
    bool is_capturing() const noexcept;
    int redirect();
    int restore() noexcept;
    int close() noexcept;

private:
    int open();

    UniqueFile file_;
    UniqueFd saved_stdout_;
    UniqueFd saved_stderr_;
    std::string temporary_path_;
    PluginStreamRedirect* previous_ = nullptr;
};

class PluginOutputCapture {
public:
    PluginOutputCapture();
    ~PluginOutputCapture();

    PluginOutputCapture(const PluginOutputCapture&) = delete;
    PluginOutputCapture& operator=(const PluginOutputCapture&) = delete;

    int status() const noexcept { return status_; }
    int restore() noexcept;

private:
    PluginStreamRedirect* owner_ = nullptr;
    int status_ = 0;
};

} // namespace uda

#endif // UDA_SERVER_PLUGIN_STREAM_REDIRECT_HPP
