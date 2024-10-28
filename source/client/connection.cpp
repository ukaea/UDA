// Create an IPv4 or IPv6 Socket Connection to the UDA server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------
#ifdef _WIN32
#  include <cctype>
#  include <winsock2.h> // must be included before connection.h to avoid macro redefinition in rpc/types.h
#endif

#include "connection.h"

#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__GNUC__) && !defined(__MINGW32__)
#  ifndef _WIN32
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#    include <sys/socket.h>
#  endif
#  include <strings.h>
#  include <unistd.h>
#else
#  include <process.h>
#  include <ws2tcpip.h>
#  define strcasecmp _stricmp
#  define sleep(DELAY) Sleep((DWORD)((DELAY) * 1E3))
#  define close(SOCK) closesocket(SOCK)
#  ifndef __MINGW32__
#    pragma comment(lib, "Ws2_32.lib")
#  endif

#  ifndef EAI_SYSTEM
#    define EAI_SYSTEM -11 /* System error returned in 'errno'. */
#  endif
#endif

#include "udaClientHostList.h"
#include "clientserver/errorLog.h"
#include "clientserver/manageSockets.h"
#include "logging/logging.h"
#include "client_config.h"

#include "authentication/updateSelectParms.h"
#include "uda/client.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaClientSSL.h>
#endif

#if defined(COMPILER_GCC) || defined(__clang__)
#  define ALLOW_UNUSED_TYPE __attribute__((__unused__))
#else
#  define ALLOW_UNUSED_TYPE
#endif

#define PORT_STRING 64

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::authentication;
using namespace uda::logging;

using namespace std::string_literals;

static int client_socket = -1;
static SOCKETLIST client_socketlist; // List of open sockets

int uda::client::connectionOpen()
{
    return client_socket != -1;
}

int uda::client::reconnect(config::Config& config, XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout)
{
    int err = 0;

    // Save current client and server timer settings, Socket and XDR handles

    time_t tv_server_start0 = *tv_server_start;
    int user_timeout0 = *user_timeout;
    int clientSocket0 = client_socket;
    XDR* clientInput0 = *client_input;
    XDR* clientOutput0 = *client_output;

    // Identify the current Socket connection in the Socket List

    int socketId = get_socket_record_id(&client_socketlist, client_socket);

    // Instance a new server if the Client has changed the host and/or port number

    bool reconnect = config.get("connection.reconnect").as_or_default(false);
    if (reconnect) {
        // RC
        int status;
        int fh;

        auto host = config.get("connection.host").as_or_default(""s);
        auto port = config.get("connection.port").as_or_default(56565);

        if (get_socket(&client_socketlist, TYPE_UDA_SERVER, &status, host, port, &fh) == 0) {
            config.set("connection.socket", fh);
            config.set("connection.change_socket", true);
        } else {
            time(tv_server_start);                 // Start a New Server AGE timer
            client_socket = -1;                    // Flags no Socket is open
            config.set("connection.change_socket", false);
        }
    }

    // Client manages connections through the Socket id and specifies which running server to connect to

    auto change_socket = config.get("connection.change_socket").as_or_default(false);
    auto socket = config.get("connection.socket").as_or_default(0);

    if (change_socket) {
        int newsocketId;
        if ((newsocketId = get_socket_record_id(&client_socketlist, socket)) < 0) {
            err = NO_SOCKET_CONNECTION;
            add_error(ErrorType::Code, __func__, err, "The User Specified Socket Connection does not exist");
            return err;
        }

        // replace with previous timer settings and XDR handles

        *tv_server_start = client_socketlist.sockets[newsocketId].tv_server_start;
        *user_timeout = client_socketlist.sockets[newsocketId].user_timeout;
        client_socket = client_socketlist.sockets[newsocketId].fh;
        *client_input = client_socketlist.sockets[newsocketId].Input;
        *client_output = client_socketlist.sockets[newsocketId].Output;

        config.set("connection.socket", client_socketlist.sockets[newsocketId].fh);
        config.set("connection.host", (const char*)client_socketlist.sockets[newsocketId].host);
        config.set("connection.port", client_socketlist.sockets[newsocketId].port);
        config.set("connection.change_socket", false);
    }

    // save Previous data if a previous socket existed

    if (socketId >= 0) {
        client_socketlist.sockets[socketId].tv_server_start = tv_server_start0;
        client_socketlist.sockets[socketId].user_timeout = user_timeout0;
        client_socketlist.sockets[socketId].fh = clientSocket0;
        client_socketlist.sockets[socketId].Input = clientInput0;
        client_socketlist.sockets[socketId].Output = clientOutput0;
    }

    return err;
}

void localhostInfo(int* ai_family)
{
    char addr_buf[64];
    struct addrinfo *info = nullptr, *result = nullptr;
    getaddrinfo("localhost", nullptr, nullptr, &info);
    result = info; // Take the first linked list member
    if (result->ai_family == AF_INET) {
        *ai_family = AF_INET;
        inet_ntop(AF_INET, &((struct sockaddr_in*)result->ai_addr)->sin_addr, addr_buf, sizeof(addr_buf));
        UDA_LOG(UDA_LOG_DEBUG, "localhost Information: IPv4 - {}", addr_buf);
    } else {
        *ai_family = AF_INET6;
        inet_ntop(AF_INET6, &((struct sockaddr_in6*)result->ai_addr)->sin6_addr, addr_buf, sizeof(addr_buf));
        UDA_LOG(UDA_LOG_DEBUG, "localhost Information: IPv6 - {}", addr_buf);
    }
    if (info) {
        freeaddrinfo(info);
    }
}

void setHints(struct addrinfo* hints, const char* hostname)
{
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = 0; // AI_CANONNAME | AI_V4MAPPED | AI_ALL | AI_ADDRCONFIG ;
    hints->ai_protocol = 0;
    hints->ai_canonname = nullptr;
    hints->ai_addr = nullptr;
    hints->ai_next = nullptr;

    // RC Fix IPv6 connection for localhost
    hints->ai_family = AF_INET;
    return;

    // Localhost? Which IP family? (AF_UNSPEC gives an 'Unknown Error'!)

    if (!strcmp(hostname, "localhost")) {
        localhostInfo(&hints->ai_family);
    }

    // Is the address Numeric? Is it IPv6?

    if (strchr(hostname, ':')) { // Appended port number should have been stripped off
        hints->ai_family = AF_INET6;
        hints->ai_flags = hints->ai_flags | AI_NUMERICHOST;
    } else {
        // make a list of address components
        std::vector<std::string> list;
        boost::split(list, hostname, boost::is_any_of("."), boost::token_compress_on);

        // Are all address components numeric?
        bool isNumeric = true;
        for (const auto& token : list) {
            size_t lstr = token.size();
            for (size_t j = 0; j < lstr; j++) {
                isNumeric &= (bool)std::isdigit(token[j]);
            }
        }

        if (isNumeric) {
            hints->ai_flags = hints->ai_flags | AI_NUMERICHOST;
        }
    }
}

int uda::client::createConnection(XDR* client_input, XDR* client_output, time_t* tv_server_start, int user_timeout)
{
    int window_size = DB_READ_BLOCK_SIZE; // 128K
    int rc;

    static int max_socket_delay = -1;
    static int max_socket_attempts = -1;

    if (max_socket_delay < 0) {
        char* env = getenv("UDA_MAX_SOCKET_DELAY");
        if (env == nullptr) {
            max_socket_delay = 10;
        } else {
            max_socket_delay = (int)strtol(env, nullptr, 10);
        }
    }

    if (max_socket_attempts < 0) {
        char* env = getenv("UDA_MAX_SOCKET_ATTEMPTS");
        if (env == nullptr) {
            max_socket_attempts = 3;
        } else {
            max_socket_attempts = (int)strtol(env, nullptr, 10);
        }
    }

    if (client_socket >= 0) {
        // Check Already Opened?
        return 0;
    }

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putUdaClientSSLSocket(client_socket);
#endif

#ifdef _WIN32 // Initialise WINSOCK Once only
    static unsigned int initWinsock = 0;
    WORD sockVersion;
    WSADATA wsaData;
    if (!initWinsock) {
        sockVersion = MAKEWORD(2, 2); // Select Winsock version 2.2
        WSAStartup(sockVersion, &wsaData);
        initWinsock = 1;
    }
#endif

    // Identify the UDA server host and is the socket IPv4 or IPv6?

    auto config = client_config();

    auto host_name = config->get("connection.host").as_or_default(""s);
    char service_port[PORT_STRING];

    // Check if the host_name is an alias for an IP address or domain name in the client configuration - replace if
    // found

    auto host = udaClientFindHostByAlias(host_name.c_str());
    if (host != nullptr) {
        if (host->host_name.empty()) {
            add_error(ErrorType::Code, __func__, -1,
                      "The host_name is not recognised for the host alias provided!");
            return -1;
        }
        host_name = host->host_name;

        auto config_host = config->get("connection.host").as_or_default(""s);

        if (config_host != host_name) {
            config->set("connection.host", host_name);
        }

        auto config_port = config->get("connection.port").as_or_default(0);
        int port = host->port;
        if (port > 0 && config_port != port) {
            config->set("connection.port", port);
        }
    } else if ((host = udaClientFindHostByName(host_name.c_str())) != nullptr) {
        // No alias found, maybe the domain name or ip address is listed
        int port = host->port;
        auto config_port = config->get("connection.port").as_or_default(0);
        if (port > 0 && config_port != port) {
            config->set("connection.port", port);
        }
    }
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putClientHost(host);
#endif

    auto config_port = config->get("connection.port").as_or_default(0);
    snprintf(service_port, PORT_STRING, "%d", config_port);

    // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if (boost::starts_with(host_name, "SSL://")) {
        // Should be stripped already if via the HOST client configuration file
        config->set("connection.host", host_name.substr(6));
        putUdaClientSSLProtocol(1);
    } else {
        if (host != nullptr && host->isSSL) {
            putUdaClientSSLProtocol(1);
        } else {
            putUdaClientSSLProtocol(0);
        }
    }
#endif

    // Resolve the Host and the IP protocol to be used (Hints not used)

    struct addrinfo* result = nullptr;
    struct addrinfo hints = {0};
    setHints(&hints, host_name.c_str());

    errno = 0;
    // RC if ((rc = getaddrinfo(hostname, service_port, &hints, &result)) != 0 || (errno != 0 && errno != ESRCH)) {
    if ((rc = getaddrinfo(host_name.c_str(), service_port, &hints, &result)) != 0) {
        add_error(ErrorType::System, __func__, rc, (char*)gai_strerror(rc));
        if (rc == EAI_SYSTEM || errno != 0) {
            add_error(ErrorType::System, __func__, errno, "");
        }
        if (result) {
            freeaddrinfo(result);
        }
        return -1;
    }

    if (result->ai_family == AF_INET) {
        UDA_LOG(UDA_LOG_DEBUG, "Socket Connection is IPv4");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Socket Connection is IPv6");
    }

    errno = 0;
    client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (client_socket < 0 || errno != 0) {
        if (errno != 0) {
            add_error(ErrorType::System, __func__, errno, "");
        } else {
            add_error(ErrorType::Code, __func__, -1, "Problem Opening Socket");
        }
        if (client_socket != -1)
#ifndef _WIN32
        {
            close(client_socket);
        }
#else
            closesocket(client_socket);
#endif
        client_socket = -1;
        freeaddrinfo(result);
        return -1;
    }

    // Connect to server

    errno = 0;
    while ((rc = connect(client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

    if (rc < 0 || (errno != 0 && errno != EINTR)) {

        // Try again for a maximum number of tries with a random time delay between attempts

        int ps;
        ps = getpid();
        srand((unsigned int)ps); // Seed the random number generator with the process id
        unsigned int delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0; // random delay
        sleep(delay);
        errno = 0;                                      // wait period
        for (int i = 0; i < max_socket_attempts; i++) { // try again
            while ((rc = connect(client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

            if (rc == 0 && errno == 0) {
                break;
            }

            delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;
            sleep(delay); // wait period
        }

        if (rc != 0 || errno != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Connect errno = {}", errno);
            UDA_LOG(UDA_LOG_DEBUG, "Connect rc = {}", rc);
            UDA_LOG(UDA_LOG_DEBUG, "Unable to connect to primary host: {} on port {}", host_name, service_port);
        }

        auto config_host = config->get("connection.host").as_or_default(""s);
        auto config_host2 = config->get("connection.host2").as_or_default(""s);

        // Abandon the principal Host - attempt to connect to the secondary host
        if (rc < 0 && config_host != config_host2 && !config_host2.empty()) {

            freeaddrinfo(result);
            result = nullptr;
#ifndef _WIN32
            close(client_socket);
#else
            closesocket(client_socket);
#endif
            client_socket = -1;
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            putClientHost(nullptr);
#endif
            host_name = config_host2;

            // Check if the host_name is an alias for an IP address or name in the client configuration - replace if
            // found

            auto config_port2 = config->get("connection.port2").as_or_default(0);

            host = udaClientFindHostByAlias(host_name.c_str());
            if (host != nullptr) {
                if (host->host_name.empty()) {
                    add_error(ErrorType::Code, __func__, -1,
                              "The hostname2 is not recognised for the host alias provided!");
                    return -1;
                }
                host_name = host->host_name;
                if (config_host2 != host_name) {
                    config->set("connection.host2", host_name);
                }
                int port = host->port;
                if (port > 0 && config_port2 != port) {
                    config->set("connection.port2", port);
                }
            } else if ((host = udaClientFindHostByName(host_name.c_str())) != nullptr) { // No alias found
                int port = host->port;
                if (port > 0 && config_port2 != port) {
                    config->set("connection.port2", port);
                }
            }
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            putClientHost(host);
#endif

            config_port2 = config->get("connection.port2").as_or_default(0);
            snprintf(service_port, PORT_STRING, "%d", config_port2);

            // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            if (boost::starts_with(host_name, "SSL://")) {
                // Should be stripped already if via the HOST client configuration file
                config->set("connection.host2", host_name.substr(6));
                putUdaClientSSLProtocol(1);
            } else {
                if (host != nullptr && host->isSSL) {
                    putUdaClientSSLProtocol(1);
                } else {
                    putUdaClientSSLProtocol(0);
                }
            }
#endif

            // Resolve the Host and the IP protocol to be used (Hints not used)

            setHints(&hints, host_name.c_str());

            errno = 0;
            if ((rc = getaddrinfo(host_name.c_str(), service_port, &hints, &result)) != 0 || errno != 0) {
                add_error(ErrorType::System, __func__, rc, (char*)gai_strerror(rc));
                if (rc == EAI_SYSTEM || errno != 0) {
                    add_error(ErrorType::System, __func__, errno, "");
                }
                if (result) {
                    freeaddrinfo(result);
                }
                return -1;
            }
            errno = 0;
            client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

            if (client_socket < 0 || errno != 0) {
                if (errno != 0) {
                    add_error(ErrorType::System, __func__, errno, "");
                } else {
                    add_error(ErrorType::Code, __func__, -1, "Problem Opening Socket");
                }
                if (client_socket != -1)
#ifndef _WIN32
                {
                    close(client_socket);
                }
#else
                    closesocket(client_socket);
#endif
                client_socket = -1;
                freeaddrinfo(result);
                return -1;
            }

            for (int i = 0; i < max_socket_attempts; i++) {
                while ((rc = connect(client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}
                if (rc == 0) {
                    int port = config->get("connection.port").as_or_default(0);
                    int port2 = config->get("connection.port2").as_or_default(0);

                    auto host = config->get("connection.host").as_or_default(""s);
                    auto host2 = config->get("connection.host2").as_or_default(""s);

                    config->set("connection.host", host2);
                    config->set("connection.host2", host);

                    config->set("connection.port", port2);
                    config->set("connection.port2", port);
                    break;
                }
                delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0;
                sleep(delay); // wait period
            }
        }

        if (rc < 0) {
            if (errno != 0) {
                add_error(ErrorType::System, __func__, errno, "");
            } else {
                add_error(ErrorType::Code, __func__, -1, "Unable to Connect to Server Stream Socket");
            }
            if (client_socket != -1)
#ifndef _WIN32
            {
                close(client_socket);
            }
#else
                closesocket(client_socket);
#endif
            client_socket = -1;
            if (result) {
                freeaddrinfo(result);
            }
            return -1;
        }
    }

    if (result) {
        freeaddrinfo(result);
    }

    // Set the receive and send buffer sizes

    setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, (char*)&window_size, sizeof(window_size));

    setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, (char*)&window_size, sizeof(window_size));

    // Other Socket Options

    int on = 1;
    if (setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        add_error(ErrorType::Code, __func__, -1, "Error Setting KEEPALIVE on Socket");
        close(client_socket);
        client_socket = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) < 0) {
        add_error(ErrorType::Code, __func__, -1, "Error Setting NODELAY on Socket");
        close(client_socket);
        client_socket = -1;
        return -1;
    }

    auto config_host = config->get("connection.host").as_or_default(""s);
    config_port = config->get("connection.port").as_or_default(0);

    // Add New Socket to the Socket's List
    add_socket(&client_socketlist, TYPE_UDA_SERVER, 1, config_host, config_port,
               client_socket);
    client_socketlist.sockets[get_socket_record_id(&client_socketlist, client_socket)].Input = client_input;
    client_socketlist.sockets[get_socket_record_id(&client_socketlist, client_socket)].Output = client_output;
    client_socketlist.sockets[get_socket_record_id(&client_socketlist, client_socket)].user_timeout = user_timeout;
    client_socketlist.sockets[get_socket_record_id(&client_socketlist, client_socket)].tv_server_start =
        *tv_server_start;

    config->set("connection.reconnect", false);
    config->set("connection.change_socket", false);
    config->set("connection.socket", client_socket);

    // Write the socket number to the SSL functions

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putUdaClientSSLSocket(client_socket);
#endif

    return 0;
}

void udaCloseAllConnections()
{
    closeConnection(ClosedownType::CLOSE_ALL);
}

void uda::client::closeConnection(ClosedownType type)
{
    if (client_socket >= 0 && type != ClosedownType::CLOSE_ALL) {
        close_client_socket(&client_socketlist, client_socket);
    } else {
        close_client_sockets(&client_socketlist);
    }

    client_socket = -1;
}

int uda::client::clientWriteout(void* iohandle ALLOW_UNUSED_TYPE, char* buf, int count)
{
#ifndef _WIN32
    void (*OldSIGPIPEHandler)(int);
#endif
    int rc = 0;
    size_t BytesSent = 0;

    fd_set wfds;
    struct timeval tv = {};

    udaUpdateSelectParms(client_socket, &wfds, &tv);

    errno = 0;

    while (select(client_socket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                UDA_LOG(UDA_LOG_DEBUG, "ECONNRESET error!");
                add_error(ErrorType::Code, __func__, -2,
                          "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    UDA_LOG(UDA_LOG_DEBUG, "ENETUNREACH error!");
                    add_error(ErrorType::Code, __func__, -3, "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    UDA_LOG(UDA_LOG_DEBUG, "ECONNREFUSED error!");
                    add_error(ErrorType::Code, __func__, -4, "Server Unavailable: ECONNREFUSED");
                    return -4;
                }
            }
        }

        udaUpdateSelectParms(client_socket, &wfds, &tv);
    }

    /* UNIX version

     Ignore the SIG_PIPE signal.  Standard behaviour terminates
     the application with an error code of 141.  When the signal
     is ignored, write calls (when there is no server process to
     communicate with) will return with errno set to EPIPE
    */

#ifndef _WIN32
    if ((OldSIGPIPEHandler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
        add_error(ErrorType::Code, __func__, -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }
#endif

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < (unsigned int)count) {
#ifndef _WIN32
        while (((rc = (int)write(client_socket, buf, count)) == -1) && (errno == EINTR)) {}
#else
        while (((rc = send(client_socket, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
#endif
        BytesSent += rc;
        buf += rc;
    }

    // Restore the original SIGPIPE handler set by the application

#ifndef _WIN32
    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        add_error(ErrorType::Code, __func__, -1, "Error attempting to restore SIG_PIPE handler");
        return -1;
    }
#endif

    return rc;
}

int uda::client::clientReadin(void* iohandle ALLOW_UNUSED_TYPE, char* buf, int count)
{
    int rc;
    fd_set rfds;
    struct timeval tv = {};

    int maxloop = 0;

    errno = 0;

    /* Wait till it is possible to read from socket */

    udaUpdateSelectParms(client_socket, &rfds, &tv);

    while ((select(client_socket + 1, &rfds, nullptr, nullptr, &tv) <= 0) && maxloop++ < MAXLOOP) {
        udaUpdateSelectParms(client_socket, &rfds, &tv); // Keep trying ...
    }

    // Read from it, checking for EINTR, as happens if called from IDL

#ifndef _WIN32
    while (((rc = (int)read(client_socket, buf, count)) == -1) && (errno == EINTR)) {}
#else
    while (((rc = recv(client_socket, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
#endif

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
        if (errno != 0 && errno != EINTR) {
            add_error(ErrorType::System, __func__, rc, "");
        }
        add_error(ErrorType::Code, __func__, rc, "No Data waiting at Socket when Data Expected!");
    }

    return rc;
}
