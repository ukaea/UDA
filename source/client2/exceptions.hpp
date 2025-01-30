#pragma once

#include <fmt/format.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace uda::exceptions
{

class UDAException : std::exception
{
  public:
    UDAException(std::string_view msg) { msg_ = msg; }

    template <class... Args> UDAException(std::string_view msg, Args... args)
    {
        msg_ = fmt::format(msg, args...);
    }

    const char* what() const noexcept override { return msg_.c_str(); }

  protected:
    std::string msg_;
};

class ClientError : public UDAException
{
  public:
    ClientError(std::string_view msg) : UDAException(msg) {}

    template <class... Args> ClientError(std::string_view msg, Args... args) : UDAException(msg, args...) {}
};

class ServerError : public UDAException
{
  public:
    ServerError(std::string_view msg) : UDAException(msg) {}

    template <class... Args> ServerError(std::string_view msg, Args... args) : UDAException(msg, args...) {}
};

} // namespace uda::exceptions
