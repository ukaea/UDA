#include "host_list.hpp"

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

#include "logging/logging.h"

using namespace uda::logging;

const uda::client_server::HostData* uda::client::HostList::find_by_alias(std::string_view alias) const
{
    for (const auto& data : hosts_) {
        if (boost::iequals(data.host_alias, alias)) {
            return &data;
        }
    }

    return nullptr;
}

const uda::client_server::HostData* uda::client::HostList::find_by_name(std::string_view name) const
{
    std::string_view target = name;
    if (name.find("SSL://") == 0) {
        target = &name[6]; // Host name must be stripped of SSL:// prefix
    }

    for (const auto& data : hosts_) {
        if (boost::iequals(data.host_name, target)) {
            return &data;
        }
    }

    return nullptr;
}

uda::client::HostList::HostList()
{
    //----------------------------------------------------------------------------------------------------------------------
    // Read the host configuration file from default locations: No error if the file does not exist

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
    load_config_file(config_file);
}

uda::client::HostList::HostList(std::string_view config_file)
{
    load_config_file(config_file);
}

void uda::client::HostList::load_list_from_toml(uda::config::Config& config)
{
    if (!config){
        return;
    }

    // assume we require at least a host_name and an alias so skip records where this isn't set
    // all other fields considered optional
    const auto config_host_list = config.get_array("host_list");
    for (const auto& entry_map: config_host_list)
    {
        uda::client_server::HostData new_data = {};
        if (entry_map.find("host_name") == entry_map.end()) {
            continue;
        }
        new_data.host_name = entry_map.at("host_name").as<std::string>();

        if (entry_map.find("host_alias") == entry_map.end()) {
            continue;
        }
        new_data.host_alias = entry_map.at("host_alias").as<std::string>();

        if (entry_map.find("port") != entry_map.end()) {
            new_data.port = entry_map.at("port").as<int>();
        }
        if (entry_map.find("certificate") != entry_map.end()) {
            new_data.certificate = entry_map.at("certificate").as<std::string>();
        }
        if (entry_map.find("ca_certificate") != entry_map.end()) {
            new_data.ca_certificate = entry_map.at("ca_certificate").as<std::string>();
        }
        if (entry_map.find("key") != entry_map.end()) {
            new_data.key = entry_map.at("key").as<std::string>();
        }
        hosts_.emplace_back(new_data); 
    }
}


void uda::client::HostList::load_config_file(std::string_view config_file)
{
    std::ifstream conf( (std::string(config_file)) );
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

    bool new_host = false;
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

        if (boost::iequals(name, "host_name")) {
            // Trigger a new set of attributes
            if (new_host) {
                hosts_.push_back(new_data);
                new_data = {};
            }
            new_host = false;
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.host_name = tokens[1];
                new_host = true;
            }
        } else if (new_host && boost::iequals(name, "host_alias")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.host_alias = tokens[1];
            }
        } else if (new_host && boost::iequals(name, "port")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.port = std::stoi(tokens[1]);
            }
        } else if (new_host && boost::iequals(name, "certificate")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.certificate = tokens[1];
            }
        } else if (new_host && boost::iequals(name, "privateKey")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.key = tokens[1];
            }
        } else if (new_host && boost::iequals(name, "CA-Certificate")) {
            if (tokens.size() > 1 && !tokens[1].empty()) {
                new_data.ca_certificate = tokens[1];
            }
        }
    }

    if (new_host) {
        hosts_.push_back(new_data);
    }

    // Flag the connection as with SSL Authentication
    for (auto& data : hosts_) {
        if (!data.certificate.empty() && !data.key.empty() && !data.ca_certificate.empty()) {
            data.isSSL = true;
        }

        if (boost::starts_with(data.host_name, "SSL://")) {
            data.isSSL = true;
            data.host_name = data.host_name.substr(6);
        }
    }

    // Extract and Strip the port number from the host name (a.b.c:9999, localhost:9999)
    for (auto& data : hosts_) {
        size_t p = 0;
        if ((boost::iequals(data.host_name, "localhost") || data.host_name.find('.') != std::string::npos) &&
            (p = data.host_name.find(':')) != std::string::npos && data.host_name.size() + 1 < p) {
            data.port = atoi(&data.host_name[p]);
            data.host_name.resize(p);
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Number of named hosts {}", hosts_.size());
    int i = 0;
    for (const auto& data : hosts_) {
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Host Alias     : {}", i, data.host_alias.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Host Name      : {}", i, data.host_name.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Host Port      : {}", i, data.port);
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Certificate    : {}", i, data.certificate.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[{}] Key            : {}", i, data.key.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[{}] CA Certificate : {}", i, data.ca_certificate.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "[{}] isSSL          : {}", i, data.isSSL);
        ++i;
    }
}
