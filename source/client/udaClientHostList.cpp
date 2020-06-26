/*---------------------------------------------------------------
* Identify the Server Host Attributes
* Is user authentication over SSL?
*---------------------------------------------------------------------------------------------------------------------*/

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <vector>
#include <string>

#ifndef _WIN32
#  include <strings.h>
#endif

#include <client/udaClientHostList.h>
#include <clientserver/stringUtils.h>
#include <logging/logging.h>
#include <fstream>

static std::vector<HostData> hostlist;
static int hostId = -1;

void udaClientPutHostNameId(int id)
{
    hostId = id;
}

int udaClientGetHostNameId()
{
    return hostId;
}

void udaClientFreeHostList()
{
    hostlist.clear();
}

void udaClientInitHostData(HOSTDATA* host)
{
    host->hostalias[0] = '\0';
    host->hostname[0] = '\0';
    host->port = 0;
    host->certificate[0] = '\0';
    host->key[0] = '\0';
    host->ca_certificate[0] = '\0';
    host->isSSL = 0;
}

int udaClientFindHostByAlias(const char* alias)
{
    udaClientInitHostList();

    int i = 0;
    for (const auto& data : hostlist) {
        if (STR_IEQUALS(data.hostalias, alias)) {
            return i;
        }
        ++i;
    }
    return -1;
}

int udaClientFindHostByName(const char* name)
{
    udaClientInitHostList();

    const char* target = name;
    if (strcasestr(name, "SSL://")) {
        target = &name[6];    // Host name must be stripped of SSL:// prefix
    }

    int i = 0;
    for (const auto& data : hostlist) {
        if (STR_IEQUALS(data.hostname, target)) {
            return i;
        }
        ++i;
    }
    return -1;
}

char* udaClientGetHostName(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].hostname;
    } else {
        return nullptr;
    }
}

char* udaClientGetHostAlias(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].hostalias;
    } else {
        return nullptr;
    }
}

int udaClientGetHostPort(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].port;
    } else {
        return -1;
    }
}

char* udaClientGetHostCertificatePath(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].certificate;
    } else {
        return nullptr;
    }
}

char* udaClientGetHostKeyPath(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].key;
    } else {
        return nullptr;
    }
}

char* udaClientGetHostCAPath(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].ca_certificate;
    } else {
        return nullptr;
    }
}

int udaClientGetHostSSL(int id)
{
    if (id >= 0 && id < (int)hostlist.size()) {
        return hostlist[id].isSSL;
    } else {
        return 0;
    }
}

void udaClientInitHostList()
{
    static bool hostListInitialised = false;

    if (hostListInitialised) {
        return;
    }
    hostListInitialised = true;

    //----------------------------------------------------------------------------------------------------------------------
    // Read the host configuration file: No error if the file does not exist

    // Locate the hosts registration file

    constexpr const char* filename = "hosts.cfg"; // Default name
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

    errno = 0;
    FILE* conf = nullptr;
    if ((conf = fopen(config_file.c_str(), "r")) == nullptr || errno != 0) {
        if (conf != nullptr) {
            fclose(conf);
        }
        return;
    }

    // organisation: sets of 1-6 records, empty records ignored, comment begins #
    // hostName must be the first record in a set
    // hostAlias and other attributes are not required
    // ordering is not important

    // The hostname may be either a resolvable name or a numeric IP address. The latter may be in either IPv4 or IPv6 format.

    // The port number must be given separately from the IP address if the format is IPv6
    // The port number may be appended to the host name or IPv4 numeric address using the standard convention host:port pattern

    // if the host IP address or name is prefixed with SSL:// this is stripped off and the isSSL bool set true
    // if the certificates and private key are defined, the isSSL bool set true

    std::string line;
    char buffer[HOST_STRING];

    bool newHost = false;
    HostData new_data = {};

    while (fgets(buffer, HOST_STRING, conf) != nullptr) {
        convertNonPrintable2(buffer);                // convert non printable chars to spaces
        LeftTrimString(TrimString(buffer));                // remove leading and trailing spaces

        if (buffer[0] == '#') continue;
        if (strlen(buffer) == 0) continue;

        char* next = buffer;
        char* split = strchr(next, ' ');            // Split the string on the first space character
        if (split != nullptr) split[0] = '\0';          // Extract the attribute name
        LeftTrimString(TrimString(next));

        if (StringIEquals(next, "hostName")) {            // Trigger a new set of attributes
            if (newHost) {
                hostlist.push_back(new_data);
                new_data = {};
            }
            newHost = false;
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                strcpy(new_data.hostname, next);
                newHost = true;
            }
        } else if (newHost && StringIEquals(next, "hostAlias")) {
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                strcpy(new_data.hostalias, next);
            }
        } else if (newHost && StringIEquals(next, "port")) {
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                new_data.port = atoi(next);
            }
        } else if (newHost && StringIEquals(next, "certificate")) {
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                strcpy(new_data.certificate, next);
            }
        } else if (newHost && StringIEquals(next, "privateKey")) {
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                strcpy(new_data.key, next);
            }
        } else if (newHost && StringIEquals(next, "CA-Certificate")) {
            next = &split[1];
            LeftTrimString(TrimString(next));
            if (next[0] != '\0' && strlen(next) < HOST_STRING) {
                strcpy(new_data.ca_certificate, next);
            }
        }
    }

    if (newHost) {
        hostlist.push_back(new_data);
    }

    fclose(conf);

    // Flag the connection as with SSL Authentication
    for (auto& data : hostlist) {
        if (data.certificate[0] != '\0' && data.key[0] != '\0' && data.ca_certificate[0] != '\0') {
            data.isSSL = 1;
        }

        const char* p = strcasestr(data.hostname, "SSL://");
        if (p && p == data.hostname && strlen(p) > 6) {
            data.isSSL = 1;
            strcpy(data.hostname, &data.hostname[6]); // Strip prefix
        }
    }

    // Extract and Strip the port number from the host name (a.b.c:9999, localhost:9999)
    for (auto& data : hostlist) {
        char* p = nullptr;
        if ((!strcmp(data.hostname, "localhost")
             || (p = strchr(data.hostname, '.')) != nullptr)
            && (p = strrchr(data.hostname, ':')) != nullptr
            && p[1] != '\0') {
            data.port = atoi(&p[1]);
            p[0] = '\0';
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Number of named hosts %d\n", hostlist.size());
    int i = 0;
    for (const auto& data : hostlist) {
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Alias     : %s\n", i, data.hostalias);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Name      : %s\n", i, data.hostname);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Host Port      : %d\n", i, data.port);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Certificate    : %s\n", i, data.certificate);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] Key            : %s\n", i, data.key);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] CA Certificate : %s\n", i, data.ca_certificate);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] isSSL          : %d\n", i, data.isSSL);
        ++i;
    }
}
