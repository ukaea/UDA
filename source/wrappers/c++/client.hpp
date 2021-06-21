#ifndef UDA_WRAPPERS_CPP_CLIENT_H
#define UDA_WRAPPERS_CPP_CLIENT_H

#include <string>
#include <vector>
#include <exception>
#include <clientserver/export.h>

#include "UDA.hpp"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#    pragma warning(disable: 4275)
#  endif
#endif

namespace uda {

class Array;

class LIBRARY_API UDAException : public std::exception
{
public:
    UDAException(std::string what, std::vector<std::string> backtrace)
            : what_(std::move(what))
            , backtrace_(std::move(backtrace))
    {
        for (const auto& str : backtrace_) {
            backtrace_msg_ += str + "\n";
        }
    };

    explicit UDAException(std::string what) throw()
            : what_(std::move(what))
    {}

    UDAException(const UDAException& ex) noexcept : what_(ex.what_) {}
    UDAException(UDAException&& ex) noexcept : what_(std::move(ex.what_)) {}
    UDAException& operator=(const UDAException& ex) noexcept { what_ = ex.what_; return *this; }
    UDAException& operator=(UDAException&& ex) noexcept { what_ = ex.what_; ex.what_.clear(); return *this; }

    ~UDAException() noexcept override = default;

    const char* what() const noexcept override
    {
        if (!backtrace_.empty()) {
            return backtrace_msg_.c_str();
        } else {
            return what_.c_str();
        }
    }

    const char* backtrace() const
    {
        return backtrace_msg_.c_str();
    }
private:
    std::string what_;
    std::vector<std::string> backtrace_;
    std::string backtrace_msg_;
};

class LIBRARY_API ProtocolException : public UDAException {
public:
    ProtocolException(std::string what, std::vector<std::string> backtrace) : UDAException(what, backtrace)
    {}

    explicit ProtocolException(std::string what) throw() : UDAException(what)
    {}
};

class LIBRARY_API ServerException : public UDAException {
public:
    ServerException(std::string what, std::vector<std::string> backtrace) : UDAException(what, backtrace)
    {}

    explicit ServerException(std::string what) throw() : UDAException(what)
    {}
};

class LIBRARY_API InvalidUseException : public UDAException {
public:
    InvalidUseException(std::string what, std::vector<std::string> backtrace) : UDAException(what, backtrace)
    {}

    explicit InvalidUseException(std::string what) throw() : UDAException(what)
    {}
};

enum Property
{
    PROP_DATADBLE,
    PROP_DIMDBLE,
    PROP_TIMEDBLE,
    PROP_BYTES,
    PROP_BAD,
    PROP_META,
    PROP_ASIS,
    PROP_UNCAL,
    PROP_NOTOFF,
    PROP_SYNTHETIC,
    PROP_SCALAR,
    PROP_NODIMDATA,
    PROP_TIMEOUT,
    PROP_VERBOSE,
    PROP_DEBUG,
    PROP_ALTDATA,
    PROP_ALTRANK
};

enum ErrorCodes
{
    OK = 0,
};

class Result;
class IdamException;
class Signal;

class LIBRARY_API Client {
public:
    ~Client();

    Client() : results_()
    {}

    static void setProperty(Property prop, bool value);
    static void setProperty(Property prop, int value);
    static int property(Property prop);

    static void setServerHostName(const std::string& hostName);
    static void setServerPort(int portNumber);

    static std::string serverHostName();
    static int serverPort();

    const uda::Result& get(const std::string& signalName, const std::string& dataSource);
    const uda::Result& get(const std::vector<std::string> signals, const std::string& source);
    const uda::Result& get(const std::vector<std::pair<std::string, std::string>> requests);

    void put(const uda::Signal& putdata);

    void put(const std::string& instruction, char data);
    void put(const std::string& instruction, int8_t data);
    void put(const std::string& instruction, int16_t data);
    void put(const std::string& instruction, int32_t data);
    void put(const std::string& instruction, int64_t data);
    void put(const std::string& instruction, uint8_t data);
    void put(const std::string& instruction, uint16_t data);
    void put(const std::string& instruction, uint32_t data);
    void put(const std::string& instruction, uint64_t data);
    void put(const std::string& instruction, float data);
    void put(const std::string& instruction, double data);

    void put(const std::string& instruction, const std::vector<char>& data);
    void put(const std::string& instruction, const std::vector<int8_t>& data);
    void put(const std::string& instruction, const std::vector<int16_t>& data);
    void put(const std::string& instruction, const std::vector<int32_t>& data);
    void put(const std::string& instruction, const std::vector<int64_t>& data);
    void put(const std::string& instruction, const std::vector<uint8_t>& data);
    void put(const std::string& instruction, const std::vector<uint16_t>& data);
    void put(const std::string& instruction, const std::vector<uint32_t>& data);
    void put(const std::string& instruction, const std::vector<uint64_t>& data);
    void put(const std::string& instruction, const std::vector<float>& data);
    void put(const std::string& instruction, const std::vector<double>& data);

    void put(const std::string& instruction, const uda::Array& data);

private:
    std::vector<Result *> results_;
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_CLIENT_H
