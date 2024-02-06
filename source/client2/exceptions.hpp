#pragma once

#ifndef UDA_SOURCE_CLIENT2_EXCEPTIONS_H
#  define UDA_SOURCE_CLIENT2_EXCEPTIONS_H

#  include <boost/format.hpp>
#  include <stdexcept>
#  include <string>
#  include <string_view>

namespace uda
{
namespace exceptions
{

class UDAException : std::exception
{
  public:
    UDAException(std::string_view msg) { msg_ = msg; }

    template <class... Args> UDAException(std::string_view msg, Args... args)
    {
        boost::format formatter{msg.data()};
        msg_ = format(formatter, args...);
    }

    const char* what() const noexcept override { return msg_.c_str(); }

  protected:
    std::string msg_;

    std::string format(boost::format& formatter) { return formatter.str(); }

    template <class Arg, class... Args> std::string format(boost::format& formatter, Arg arg, Args... args)
    {
        formatter = formatter % arg;
        return format(formatter, args...);
    }
};

class ClientError : UDAException
{
  public:
    ClientError(std::string_view msg) : UDAException(msg) {}

    template <class... Args> ClientError(std::string_view msg, Args... args) : UDAException(msg, args...) {}
};

class ServerError : UDAException
{
  public:
    ServerError(std::string_view msg) : UDAException(msg) {}

    template <class... Args> ServerError(std::string_view msg, Args... args) : UDAException(msg, args...) {}
};

} // namespace exceptions
} // namespace uda

#endif // UDA_SOURCE_CLIENT2_EXCEPTIONS_H
