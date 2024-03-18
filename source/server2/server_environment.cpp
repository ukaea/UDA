#include "server_environment.hpp"

#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <string>

#include "server_config.h"
#include "logging/logging.h"

using namespace uda::logging;

void uda::server::Environment::print()
{
    UDA_LOG(UDA_LOG_INFO, "Server Environment Variable values\n");
    UDA_LOG(UDA_LOG_INFO, "Log Location    : %s\n", _environment.logdir);
    UDA_LOG(UDA_LOG_INFO, "Log Write Mode  : %s\n", _environment.logmode);
    UDA_LOG(UDA_LOG_INFO, "Log Level       : %d\n", _environment.loglevel);
    UDA_LOG(UDA_LOG_INFO, "External User?  : %d\n", _environment.external_user);
    UDA_LOG(UDA_LOG_INFO, "UDA Proxy Host  : %s\n", _environment.server_proxy);
    UDA_LOG(UDA_LOG_INFO, "UDA This Host   : %s\n", _environment.server_this);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Target    : %s\n", _environment.private_path_target);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Substitute: %s\n", _environment.private_path_substitute);
}

uda::server::Environment::Environment(const Config& config) : _config{ config }
{
    //--- Read Standard Set of Environment Variables ------------------------------------

    // Log Output

    auto path = _config.get("logging.path");

    if (path) {
        strcpy(_environment.logdir, path.as<std::string>().c_str());
        strcat(_environment.logdir, "/");
    } else {
        strcpy(_environment.logdir, "/scratch/udalog/"); // Log is on Scratch
    }

    auto level_string = _config.get("logging.level");

    _environment.loglevel = UDA_LOG_NONE;
    if (level_string) {
        std::string level = level_string.as<std::string>();
        boost::to_upper(level);
        if (level == "ACCESS") {
            _environment.loglevel = UDA_LOG_ACCESS;
        } else if (level == "ERROR") {
            _environment.loglevel = UDA_LOG_ERROR;
        } else if (level == "WARN") {
            _environment.loglevel = UDA_LOG_WARN;
        } else if (level == "DEBUG") {
            _environment.loglevel = UDA_LOG_DEBUG;
        } else if (level == "INFO") {
            _environment.loglevel = UDA_LOG_INFO;
        }
    }

    // Log Output Write Mode

    auto mode = _config.get("logging.mode");
    if (mode) {
        _environment.logmode[0] = mode.as_or_default<char>('w');
    }

    //-------------------------------------------------------------------------------------------
    // API Defaults

    auto device = _config.get("server.default_device");
    if (device) {
        strcpy(_environment.api_device, device.as<std::string>().c_str());
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_DEVICE environment variable not set");
    }

    auto archive = _config.get("server.default_archive");
    if (archive) {
        strcpy(_environment.api_archive, archive.as<std::string>().c_str());
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_ARCHIVE environment variable not set");
    }

    auto delim = _config.get("server.delim");
    if (delim) {
        strcpy(_environment.api_delim, delim.as<std::string>().c_str());
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_API_DELIM environment variable not set");
    }

    auto format = _config.get("server.default_format");
    if (format) {
        strcpy(_environment.api_format, format.as<std::string>().c_str());
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_FILE_FORMAT environment variable not set");
    }

    //-------------------------------------------------------------------------------------------
    // External User?

#ifdef EXTERNAL_USER
    environment_.external_user = 1;
#else
    _environment.external_user = 0;
#endif

    auto external_user = _config.get("server.external_user");
    if (external_user) {
        _environment.external_user = external_user.as<bool>();
    }

    //-------------------------------------------------------------------------------------------
    // UDA Proxy Host: redirect ALL requests

    auto proxy_target = _config.get("server.proxy_target");
    if (proxy_target) {
        strcpy(_environment.server_proxy, proxy_target.as<std::string>().c_str());
    } else {
        _environment.server_proxy[0] = '\0';
    }

    auto proxy_source = _config.get("server.proxy_source");
    if (proxy_source) {
        strcpy(_environment.server_this, proxy_source.as<std::string>().c_str());
    } else {
        _environment.server_this[0] = '\0';
    }

    auto path_targets = _config.get("server.private_path_targets");
    auto path_substitutes = _config.get("server.private_path_substitutes");

    if (path_targets) {
        strcpy(_environment.private_path_target, path_targets.as<std::string>().c_str());
        if (path_substitutes) {
            strcpy(_environment.private_path_substitute, path_substitutes.as<std::string>().c_str());
        } else {
            _environment.private_path_substitute[0] = '\0';
        }
    } else {
        _environment.private_path_target[0] = '\0';
        _environment.private_path_substitute[0] = '\0';
    }

    _environment.initialised = 1;
}
