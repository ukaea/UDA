/*---------------------------------------------------------------
 * Identify the Server Host Attributes
 * Is user authentication over SSL?
 *---------------------------------------------------------------------------------------------------------------------*/

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#ifndef _WIN32
#  include <strings.h>
#endif

#include "client/udaClientHostList.h"
#include "logging/logging.h"

using namespace uda::logging;

static std::vector<uda::client_server::HostData> g_host_list = {};

void uda::client::udaClientFreeHostList()
{
    g_host_list.clear();
}

const uda::client_server::HostData* uda::client::udaClientFindHostByAlias(const char* alias)
{
    udaClientInitHostList();

    for (const auto& data : g_host_list) {
        if (boost::iequals(data.host_alias, alias)) {
            return &data;
        }
    }
    return nullptr;
}

const uda::client_server::HostData* uda::client::udaClientFindHostByName(const char* name)
{
    udaClientInitHostList();

    const char* target = name;
    if (strcasestr(name, "SSL://")) {
        target = &name[6]; // Host name must be stripped of SSL:// prefix
    }

    for (const auto& data : g_host_list) {
        if (boost::iequals(data.host_name, target)) {
            return &data;
        }
    }
    return nullptr;
}

void uda::client::udaClientInitHostList()
{
    static bool hostListInitialised = false;

    if (hostListInitialised) {
        return;
    }
    hostListInitialised = true;

    //----------------------------------------------------------------------------------------------------------------------
    // Read the host configuration file: No error if the file does not exist

    // Locate the hosts registration file

    constexpr const char* filename = "hosts.cfg";           // Default name
    const char* config = getenv("UDA_CLIENT_HOSTS_CONFIG"); // Host configuration file
    std::string config_file;

    if (config == nullptr) {
#ifdef _WIN32
        config_file = filename; // Local directory
#else
        const char* home = getenv("HOME");
        if (home == nullptr) {
            return;
        }

        // the UDA hidden directory in the user's home directory
        config_file = std::string{home} + "/.uda/" + filename;
#endif
    } else {
        config_file = config;
    }

    // Read the hosts file

    std::ifstream conf(config_file);
    if (!conf) {
        return;
    }

    // organisation: sets of 1-6 records, empty records ignored, comment begins #
    // hostName must be the first record in a set
    // hostAlias and other attributes are not required
    // ordering is not important

    // The host_name may be either a resolvable name or a numeric IP address. The latter may be in either IPv4 or IPv6
    // format.

    // The port number must be given separately from the IP address if the format is IPv6
    // The port number may be appended to the host name or IPv4 numeric address using the standard convention host:port
    // pattern

    // if the host IP address or name is prefixed with SSL:// this is stripped off and the isSSL bool set true
    // if the certificates and private key are defined, the isSSL bool set true

    bool newHost = false;
    uda::client_server::HostData new_data = {};

    std::string line;
    while (std::getline(conf, line)) {
        boost::trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(" "));
        std::for_each(tokens.begin(), tokens.end(), [](std::string& s) { boost::trim(s); });

        std::string name = tokens[0];

        if (boost::iequals(name, "hostName")) {
            // Trigger a new set of attributes
            if (newHost) {
                g_host_list.push_back(new_data);
                new_data = {};
            }
            newHost = false;
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.host_name = tokens[1];
                newHost = true;
            }
        } else if (newHost && boost::iequals(name, "hostAlias")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.host_alias = tokens[1];
            }
        } else if (newHost && boost::iequals(name, "port")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.port = std::stoi(tokens[1]);
            }
        } else if (newHost && boost::iequals(name, "certificate")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.certificate = tokens[1];
            }
        } else if (newHost && boost::iequals(name, "privateKey")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.key = tokens[1];
            }
        } else if (newHost && boost::iequals(name, "CA-Certificate")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.ca_certificate = tokens[1];
            }
        }
    }

    if (newHost) {
        g_host_list.push_back(new_data);
    }

    // Flag the connection as with SSL Authentication
    for (auto& data : g_host_list) {
        if (!data.certificate.empty() && !data.key.empty() && !data.ca_certificate.empty()) {
            data.isSSL = true;
        }

        if (boost::starts_with(data.host_name, "SSL://")) {
            data.isSSL = true;
            data.host_name = data.host_name.substr(6);
        }
    }

    // Extract and Strip the port number from the host name (a.b.c:9999, localhost:9999)
    for (auto& data : g_host_list) {
        size_t p;
        if ((boost::iequals(data.host_name, "localhost") || data.host_name.find('.') != std::string::npos) &&
            (p = data.host_name.find(':')) != std::string::npos && data.host_name.size() + 1 < p) {
            data.port = atoi(&data.host_name[p]);
            data.host_name.resize(p);
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Number of named hosts %d\n", g_host_list.size());
    int i = 0;
    for (const auto& data : g_host_list) {
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Alias     : %s\n", i, data.host_alias.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Name      : %s\n", i, data.host_name.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Port      : %d\n", i, data.port);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Certificate    : %s\n", i, data.certificate.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Key            : %s\n", i, data.key.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[%d] CA Certificate : %s\n", i, data.ca_certificate.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[%d] isSSL          : %d\n", i, data.isSSL);
        ++i;
    }
}
