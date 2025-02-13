#pragma once

#include <fmt/format.h>
#include <errno.h>

#include "uda_structs.h"

namespace uda::client_server
{

enum class ErrorType : int {
    None = 0,
    System = 1,
    Code = 2,
    Plugin = 3,
};

const char* format_as(ErrorType error_type);

class UdaException : public std::exception {
public:
    UdaException(const char* location, const int code, const char* msg)
        : UdaException(ErrorType::Code, location, code, msg)
    {}

    [[nodiscard]] const char* what() const noexcept override {
        return what_.c_str();
    }

protected:
    UdaException(const ErrorType error_type, const char* location, const int code, const char* msg)
        : error_type_{error_type}
        , location_{location}
        , code_{code}
        , msg_{msg} {
        what_ = fmt::format("UdaException {} ({}) {} {}", format_as(error_type_), location_, code_, msg_);
    }

    std::string what_;

private:
    ErrorType error_type_;
    std::string location_;
    int code_;
    std::string msg_;
};

class UdaSystemException final : public UdaException {
public:
    UdaSystemException(const char* location, const char* msg) : UdaException(ErrorType::System, location, errno, msg) {
        const char* err_msg = strerror(errno);
        if (err_msg != nullptr) {
            what_ = what_ + ": " + err_msg;
        }
    }

};

void print_errors(const std::vector<UdaError>& error_stack, ClientBlock client_block, RequestBlock request_block);

void print_error_stack(const std::vector<UdaError>& error_stack);

UdaError create_error(ErrorType type, const char* location, int code, const char* msg);

inline void add_error(std::vector<UdaError>& error_stack, const ErrorType type, const char* location, const int code, const char* msg) {
    error_stack.push_back(create_error(type, location, code, msg));
}

} // namespace uda::client_server

#define UDA_ADD_ERROR(STK, ERR, MSG) add_error(STK, ErrorType::Code, __func__, ERR, MSG)
#define UDA_ADD_SYS_ERROR(STK, MSG) add_error(STK, ErrorType::System, __func__, errno, MSG)
#define UDA_THROW_ERROR(STK, ERR, MSG)                                                                                      \
    add_error(STK, ErrorType::Code, __func__, ERR, MSG);                                                                \
    return ERR;

#define UDA_THROW(ERR, MSG) throw uda::client_server::UdaException(__func__, ERR, MSG)
#define UDA_SYS_THROW(MSG) throw uda::client_server::UdaSystemException(__func__, MSG)
