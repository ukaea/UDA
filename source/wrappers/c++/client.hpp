//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_CLIENT_H
#define IDAM_CLIENT_H

#include <string>
#include <vector>
#include <exception>
#include "UDA.hpp"

namespace uda {

class UDAException : public std::exception
{
public:
    UDAException(const std::string& what) : what_(what) {}
    ~UDAException() throw() {};
    const char * what() const throw() { return what_.c_str(); }
private:
    const std::string what_;
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

#endif //IDAM_CLIENT_H
