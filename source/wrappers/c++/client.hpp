#ifndef UDA_WRAPPERS_CPP_CLIENT_H
#define UDA_WRAPPERS_CPP_CLIENT_H

#include <string>
#include <vector>
#include <exception>

#include "UDA.hpp"

namespace uda {

class UDAException : public std::exception
{
public:
    explicit UDAException(std::string what, std::vector<std::string> backtrace)
            : what_(std::move(what))
            , backtrace_(std::move(backtrace))
    {};

    explicit UDAException(std::string what) throw()
            : what_(std::move(what))
    {}

    UDAException(const UDAException& ex) noexcept : what_(ex.what_) {}

#ifdef SWIG_VERSION
    UDAException(UDAException&& ex) noexcept : what_(std::move(ex.what_)) {}
    UDAException& operator=(const UDAException& ex) noexcept { what_ = ex.what_; return *this; }
    UDAException& operator=(UDAException&& ex) noexcept { what_ = ex.what_; ex.what_.clear(); return *this; }
#endif

    ~UDAException() noexcept override = default;

    const char* what() const noexcept override
    {
        return what_.c_str();
    }

    std::string backtrace() const
    {
        std::string result;
        for (const auto& str : backtrace_) {
            result += str + "\n";
        }
        return result;
    }
private:
    std::string what_;
    std::vector<std::string> backtrace_;
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

class Client
{
public:
    ~Client();
    Client() : data_() {}

    static void setProperty(Property prop, bool value) throw(UDAException);
    static void setProperty(Property prop, int value) throw(UDAException);
    static int property(Property prop) throw(UDAException);

    static void setServerHostName(const std::string& hostName);
    static void setServerPort(int portNumber);
    
    static std::string serverHostName();
    static int serverPort();

    const uda::Result& get(const std::string& signalName, const std::string& dataSource) throw(UDAException);
    void put(const uda::Signal& putdata);

private:
    std::vector<Result *> data_;
};

}

#endif // UDA_WRAPPERS_CPP_CLIENT_H
