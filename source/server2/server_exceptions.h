#pragma once

#ifndef UDA_SOURCE_SERVER2_SERVER_EXCEPTIONS_H
#  define UDA_SOURCE_SERVER2_SERVER_EXCEPTIONS_H

#  include <exception>

namespace uda
{
namespace server
{

enum class ErrorCode {
    ProtocolVersionError,
    ProtocolError,
    StartupError,
};

class Exception : public std::exception
{
  public:
    explicit Exception(ErrorCode ec) : ec_{ec}, msg_{""} {}
    explicit Exception(ErrorCode ec, const char* msg) : ec_{ec}, msg_{msg} {}
    int code() { return static_cast<int>(ec_); }
    [[nodiscard]] const char* what() const noexcept override { return msg_; }

  private:
    ErrorCode ec_;
    const char* msg_;
};

class ProtocolError : public Exception
{
  public:
    explicit ProtocolError(const char* msg) : Exception(ErrorCode::ProtocolError, msg) {}
};

class ProtocolVersionError : public Exception
{
  public:
    explicit ProtocolVersionError(const char* msg) : Exception(ErrorCode::ProtocolError, msg) {}
};

class StartupException : public Exception
{
  public:
    explicit StartupException(const char* msg) : Exception(ErrorCode::StartupError, msg) {}
};

} // namespace server
} // namespace uda

#endif // UDA_SOURCE_SERVER2_SERVER_EXCEPTIONS_H
