//
// Created by jholloc on 08/03/16.
//

#include "client.hpp"

#include <boost/format.hpp>

#include <client/accAPI.h>
#include <client/udaGetAPI.h>

#include "data.hpp"
#include "result.hpp"

void Idam::Client::setProperty(Property prop, bool value) throw(IdamException)
{
    std::string name;

    switch (prop) {
        case PROP_DATADBLE:  name = "get_datadble"; break;
        case PROP_DIMDBLE:   name = "get_dimdble"; break;
        case PROP_TIMEDBLE:  name = "get_timedble"; break;
        case PROP_BYTES:     name = "get_bytes"; break;
        case PROP_BAD:       name = "get_bad"; break;
        case PROP_META:      name = "get_meta"; break;
        case PROP_ASIS:      name = "get_asis"; break;
        case PROP_UNCAL:     name = "get_uncal"; break;
        case PROP_NOTOFF:    name = "get_notoff"; break;
        case PROP_SYNTHETIC: name = "get_synthetic"; break;
        case PROP_SCALAR:    name = "get_scalar"; break;
        case PROP_NODIMDATA: name = "get_nodimdata"; break;
        case PROP_VERBOSE:   name = "verbose"; break;
        case PROP_DEBUG:     name = "debug"; break;
        case PROP_ALTDATA:   name = "altdata"; break;

        case PROP_TIMEOUT:
        case PROP_ALTRANK:
            throw IdamException("Cannot set boolean value for non-boolean property");

        default:
            throw IdamException("Unknown property");
    }

    value ? setIdamProperty(name.c_str()) : resetIdamProperty(name.c_str());
}

void Idam::Client::setProperty(Property prop, int value) throw(IdamException)
{
    std::string name;

    switch (prop) {
        case PROP_DATADBLE:
        case PROP_DIMDBLE:
        case PROP_TIMEDBLE:
        case PROP_BYTES:
        case PROP_BAD:
        case PROP_META:
        case PROP_ASIS:
        case PROP_UNCAL:
        case PROP_NOTOFF:
        case PROP_SYNTHETIC:
        case PROP_SCALAR:
        case PROP_NODIMDATA:
        case PROP_VERBOSE:
        case PROP_DEBUG:
        case PROP_ALTDATA:
            throw IdamException("Cannot set integer value for boolean property");

        case PROP_TIMEOUT:
            name = (boost::format("timeout=%1%") % value).str();
            setIdamProperty(name.c_str());
            break;
        case PROP_ALTRANK:
            name = (boost::format("altrank=%1%") % value).str();
            setIdamProperty(name.c_str());
            break;

        default:
            throw IdamException("Unknown property");
    }
}

int Idam::Client::property(Property prop) throw(IdamException)
{
    switch (prop) {
        case PROP_DATADBLE:  return getIdamProperty("get_datadble");
        case PROP_DIMDBLE:   return getIdamProperty("get_dimdble");
        case PROP_TIMEDBLE:  return getIdamProperty("get_timedble");
        case PROP_BYTES:     return getIdamProperty("get_bytes");
        case PROP_BAD:       return getIdamProperty("get_bad");
        case PROP_META:      return getIdamProperty("get_meta");
        case PROP_ASIS:      return getIdamProperty("get_asis");
        case PROP_UNCAL:     return getIdamProperty("get_uncal");
        case PROP_NOTOFF:    return getIdamProperty("get_notoff");
        case PROP_SYNTHETIC: return getIdamProperty("get_synthetic");
        case PROP_SCALAR:    return getIdamProperty("get_scalar");
        case PROP_NODIMDATA: return getIdamProperty("get_nodimdata");
        case PROP_VERBOSE:   return getIdamProperty("verbose");
        case PROP_DEBUG:     return getIdamProperty("debug");
        case PROP_ALTDATA:   return getIdamProperty("altdata");
        case PROP_TIMEOUT:   return getIdamProperty("timeout");
        case PROP_ALTRANK:   return getIdamProperty("altrank");

        default:
            throw IdamException("Unknown property");
    }
}

void Idam::Client::setServerHostName(const std::string& hostName)
{
    putIdamServerHost(hostName.c_str());
}

void Idam::Client::setServerPort(int portNumber)
{
    putIdamServerPort(portNumber);
}

std::string Idam::Client::serverHostName()
{
    return getIdamServerHost();
}

int Idam::Client::serverPort()
{
    return getIdamServerPort();
}

const Idam::Result& Idam::Client::get(const std::string& signalName, const std::string& dataSource) throw(IdamException)
{
    Result * data = new Result(idamGetAPI(signalName.c_str(), dataSource.c_str()));

    if (data->errorCode() != OK) {
        std::string error = data->error();
        delete data;
        throw IdamException(error);
    }

    data_.push_back(data);
    return *data;
}

Idam::Client::~Client()
{
    for (std::vector<Result *>::iterator iter = data_.begin(); iter != data_.end(); ++iter) {
        delete(*iter);
    }
}
