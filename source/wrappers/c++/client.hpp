//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_CLIENT_H
#define IDAM_CLIENT_H

#include <string>
#include <vector>
#include <exception>

namespace Idam {

class IdamException : public std::exception
{
public:
    IdamException(const std::string& what) : what_(what) {}
    ~IdamException() throw() {};
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

class Client
{
public:
    ~Client();
    Client() : data_() {}

    static void setProperty(Property prop, bool value) throw(IdamException);
    static void setProperty(Property prop, int value) throw(IdamException);
    static int property(Property prop) throw(IdamException);

    static void setServerHostName(const std::string& hostName);
    static void setServerPort(int portNumber);
    
    
    static std::string serverHostName();
    static int serverPort();

    const Idam::Result& get(const std::string& signalName, const std::string& dataSource) throw(IdamException);

private:
    std::vector<Result *> data_;
};

}

#endif //IDAM_CLIENT_H
