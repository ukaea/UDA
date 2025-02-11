// Create an IPv4 or IPv6 Socket Connection to the UDA server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------
#ifdef _WIN32
#  include <cctype>
#  include <winsock2.h> // must be included before connection.h to avoid macro redefinition in rpc/types.h
#endif

#include "connection.hpp"

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

#include "clientserver/errorLog.h"
#include "clientserver/manage_sockets.h"
#include "logging/logging.h"
#include "config/config.h"

#include "host_list.hpp"
#include "uda/client.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaClientSSL.h>
using namespace uda::authentication;
#endif

#if defined(COMPILER_GCC) || defined(__clang__)
#  define ALLOW_UNUSED_TYPE __attribute__((__unused__))
#else
#  define ALLOW_UNUSED_TYPE
#endif

#define PORT_STRING 64

using namespace uda::client_server;
using namespace uda::logging;

using namespace std::string_literals;

int uda::client::Connection::open()
{
    return _client_socket != -1;
}

int uda::client::Connection::find_socket(int fh)
{
    int i = 0;
    for (const auto& socket : _socket_list) {
        if (socket.fh == _client_socket) {
            return i;
        }
        ++i;
    }
    return -1;
}

int uda::client::Connection::reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start,
                                       int* user_timeout)
{
    int err = 0;

    // Save current client and server timer settings, Socket and XDR handles

    time_t tv_server_start0 = *tv_server_start;
    int user_timeout0 = *user_timeout;
    int clientSocket0 = _client_socket;
    XDR* clientInput0 = *client_input;
    XDR* clientOutput0 = *client_output;

    // Identify the current Socket connection in the Socket List

    int socketId = find_socket(_client_socket);

    // Instance a new server if the Client has changed the host and/or port number

    auto server_reconnect = _config.get("client.server_reconnect").as_or_default(false);
    auto server_change_socket = _config.get("client.server_change_socket").as_or_default(false);

    if (server_reconnect) {
        time(tv_server_start);                // Start a New Server AGE timer
        _client_socket = -1;                   // Flags no Socket is open
        _config.set("client.server_reconnect", false);
    }

    // Client manages connections through the Socket id and specifies which running server to connect to

    if (server_change_socket) {
        auto server_socket = _config.get("client.server_socket").as_or_default(-1);
        if ((socketId = find_socket(server_socket)) < 0) {
            err = NO_SOCKET_CONNECTION;
            add_error(ErrorType::Code, __func__, err, "The User Specified Socket Connection does not exist");
            return err;
        }

        // replace with previous timer settings and XDR handles

        *tv_server_start = _socket_list[socketId].tv_server_start;
        *user_timeout = _socket_list[socketId].user_timeout;
        _client_socket = _socket_list[socketId].fh;
        *client_input = _socket_list[socketId].Input;
        *client_output = _socket_list[socketId].Output;

        _config.set("client.server_change_socket", false);
        _config.set("client.server_socket", _socket_list[socketId].fh);
        _config.set("client.server_port", _socket_list[socketId].port);
        _config.set("client.server_host", std::string{_socket_list[socketId].host});
    }

    // save Previous data if a previous socket existed

    if (socketId >= 0) {
        _socket_list[socketId].tv_server_start = tv_server_start0;
        _socket_list[socketId].user_timeout = user_timeout0;
        _socket_list[socketId].fh = clientSocket0;
        _socket_list[socketId].Input = clientInput0;
        _socket_list[socketId].Output = clientOutput0;
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

void setHints(struct addrinfo* hints, const char* host_name)
{
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = 0; // AI_CANONNAME | AI_V4MAPPED | AI_ALL | AI_ADDRCONFIG ;
    hints->ai_protocol = 0;
    hints->ai_canonname = nullptr;
    hints->ai_addr = nullptr;
    hints->ai_next = nullptr;

    // Localhost? Which IP family? (AF_UNSPEC gives an 'Unknown Error'!)

    if (!strcmp(host_name, "localhost")) {
        localhostInfo(&hints->ai_family);
    }

    // Is the address Numeric? Is it IPv6?

    if (strchr(host_name, ':')) { // Appended port number should have been stripped off
        hints->ai_family = AF_INET6;
        hints->ai_flags = hints->ai_flags | AI_NUMERICHOST;
    } else {
        // make a list of address components
        std::vector<std::string> list;
        boost::split(list, host_name, boost::is_any_of("."), boost::token_compress_on);

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

int uda::client::Connection::create(XDR* client_input, XDR* client_output, const HostList& host_list)
{
    int window_size = DBReadBlockSize; // 128K
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

    if (_client_socket >= 0) {
        // Check Already Opened?
        return 0;
    }

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putUdaClientSSLSocket(_client_socket);
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

    auto host_name = _config.get("client.server_host").as_or_default(""s);
    char service_port[PORT_STRING];

    // Check if the host_name is an alias for an IP address or domain name in the client configuration - replace if
    // found

    auto server_host = _config.get("client.server_host").as_or_default(""s);
    auto server_port = _config.get("client.server_port").as_or_default(0);

    auto host = host_list.find_by_alias(host_name);
    if (host != nullptr) {
        if (host->host_name != server_host) {
            _config.set("client.server_host", host->host_name); // Replace
        }
        int port = host->port;
        if (port > 0 && server_port != port) {
            _config.set("client.server_port", port);
            server_port = port;
        }
    } else if ((host = host_list.find_by_name(host_name)) != nullptr) {
        // No alias found, maybe the domain name or ip address is listed
        int port = host->port;
        if (port > 0 && server_port != port) {
            // Replace if found and different
            _config.set("client.server_port", port);
            server_port = port;
        }
    }
    snprintf(service_port, PORT_STRING, "%d", server_port);

    // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if (boost::starts_with(host_name, "SSL://")) {
        // Should be stripped already if via the HOST client configuration file
        auto new_host = host_name.substr(6);
        _config.set("client.server_host", new_host);
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
    if ((rc = getaddrinfo(host_name.c_str(), service_port, &hints, &result)) != 0 || (errno != 0 && errno != ESRCH)) {
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
    _client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (_client_socket < 0 || errno != 0) {
        if (errno != 0) {
            add_error(ErrorType::System, __func__, errno, "");
        } else {
            add_error(ErrorType::Code, __func__, -1, "Problem Opening Socket");
        }
        if (_client_socket != -1) {
#ifndef _WIN32
            ::close(_client_socket);
#else
            ::closesocket(_client_socket);
#endif
        }
        _client_socket = -1;
        freeaddrinfo(result);
        return -1;
    }

    // Connect to server

    errno = 0;
    while ((rc = connect(_client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

    if (rc < 0 || (errno != 0 && errno != EINTR)) {

        // Try again for a maximum number of tries with a random time delay between attempts

        int ps;
        ps = getpid();
        srand((unsigned int)ps); // Seed the random number generator with the process id
        unsigned int delay = max_socket_delay > 0 ? (unsigned int)(rand() % max_socket_delay) : 0; // random delay
        sleep(delay);
        errno = 0;                                      // wait period
        for (int i = 0; i < max_socket_attempts; i++) { // try again
            while ((rc = connect(_client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

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

        auto server_host2 = _config.get("client.server_host2").as_or_default(""s);

        // Abandon the principal Host - attempt to connect to the secondary host
        if (rc < 0 && !server_host2.empty() && server_host != server_host2) {

            freeaddrinfo(result);
            result = nullptr;
#ifndef _WIN32
            ::close(_client_socket);
#else
            ::closesocket(_client_socket);
#endif
            _client_socket = -1;
            host_name = server_host2;

            // Check if the host_name is an alias for an IP address or name in the client configuration - replace if
            // found

            auto server_port2 = _config.get("server_port2").as_or_default(0);

            host = host_list.find_by_alias(host_name);
            if (host != nullptr) {
                if (host->host_name != server_host2) {
                    server_host2 = host->host_name;
                }
                int port = host->port;
                if (port > 0 && server_port2 != port) {
                    server_port2 = port;
                }
            } else if ((host = host_list.find_by_name(host_name)) != nullptr) { // No alias found
                int port = host->port;
                if (port > 0 && server_port2 != port) {
                    server_port2 = port;
                }
            }
            snprintf(service_port, PORT_STRING, "%d", server_port2);

            // Does the host name contain the SSL protocol prefix? If so strip this off

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            if (boost::starts_with(host_name, "SSL://")) {
                // Should be stripped already if via the HOST client configuration file
                server_host2 = host_name.substr(6);
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
            _client_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

            if (_client_socket < 0 || errno != 0) {
                if (errno != 0) {
                    add_error(ErrorType::System, __func__, errno, "");
                } else {
                    add_error(ErrorType::Code, __func__, -1, "Problem Opening Socket");
                }
                if (_client_socket != -1) {
#ifndef _WIN32
                    ::close(_client_socket);
#else
                    ::closesocket(_client_socket);
#endif
                }
                _client_socket = -1;
                freeaddrinfo(result);
                return -1;
            }

            for (int i = 0; i < max_socket_attempts; i++) {
                while ((rc = connect(_client_socket, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}
                if (rc == 0) {
                    std::swap(server_port2, server_port);
                    std::swap(server_host2, server_host);
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
            if (_client_socket != -1)
#ifndef _WIN32
            {
                ::close(_client_socket);
            }
#else
                closesocket(_client_socket);
#endif
            _client_socket = -1;
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

    setsockopt(_client_socket, SOL_SOCKET, SO_SNDBUF, (char*)&window_size, sizeof(window_size));

    setsockopt(_client_socket, SOL_SOCKET, SO_RCVBUF, (char*)&window_size, sizeof(window_size));

    // Other Socket Options

    int on = 1;
    if (setsockopt(_client_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        add_error(ErrorType::Code, __func__, -1, "Error Setting KEEPALIVE on Socket");
        ::close(_client_socket);
        _client_socket = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(_client_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) < 0) {
        add_error(ErrorType::Code, __func__, -1, "Error Setting NODELAY on Socket");
        ::close(_client_socket);
        _client_socket = -1;
        return -1;
    }

    // Add New Socket to the Socket's List

    Socket socket = {};

    socket.open = true;
    socket.fh = _client_socket;
    socket.port = server_port;
    socket.host = server_host;
    socket.tv_server_start = 0;
    socket.user_timeout = 0;
    socket.Input = client_input;
    socket.Output = client_output;

    _socket_list.push_back(socket);

    _config.set("client.server_reconnect", false);
    _config.set("client.server_change_socket", false);
    _config.set("client.server_socket", _client_socket);

    // Write the socket number to the SSL functions

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    putUdaClientSSLSocket(_client_socket);
#endif

    return 0;
}

void uda::client::Connection::close_socket(const int fh)
{
    for (auto& socket : _socket_list) {
        if (socket.open && socket.fh == fh && socket.fh >= 0) {
#ifndef _WIN32
            ::close(fh); // Only Genuine Sockets!
#else
            ::closesocket(fh);
#endif
            socket.open = false;
            socket.fh = -1;
            break;
        }
    }
}

void uda::client::Connection::close_down(ClosedownType type)
{
    if (_client_socket >= 0 && type != ClosedownType::CLOSE_ALL) {
        close_socket(_client_socket);
    } else {
        for (const auto& socket : _socket_list) {
            close_socket(socket.fh);
        }
    }

    _client_socket = -1;
}

void update_select_params(int fd, fd_set* rfds, struct timeval* tv)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    tv->tv_sec = 0;
    tv->tv_usec = 500; // in microsecs => 0.5 ms wait
}

int uda::client::writeout(void* iohandle, char* buf, int count)
{
#ifndef _WIN32
    void (*OldSIGPIPEHandler)(int);
#endif
    int rc = 0;
    size_t BytesSent = 0;

    fd_set wfds;
    struct timeval tv = {};

    auto io_data = reinterpret_cast<IoData*>(iohandle);

    update_select_params(*io_data->client_socket, &wfds, &tv);

    errno = 0;

    while (select(*io_data->client_socket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {

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

        update_select_params(*io_data->client_socket, &wfds, &tv);
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
        while (((rc = (int)write(*io_data->client_socket, buf, count)) == -1) && (errno == EINTR)) {}
#else
        while (((rc = send(*io_data->_client_socket, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
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

int uda::client::readin(void* iohandle, char* buf, int count)
{
    int rc;
    fd_set rfds;
    struct timeval tv = {};

    int maxloop = 0;

    auto io_data = reinterpret_cast<IoData*>(iohandle);

    errno = 0;

    /* Wait till it is possible to read from socket */

    update_select_params(*io_data->client_socket, &rfds, &tv);

    while ((select(*io_data->client_socket + 1, &rfds, nullptr, nullptr, &tv) <= 0) && maxloop++ < MaxLoop) {
        update_select_params(*io_data->client_socket, &rfds, &tv); // Keep trying ...
    }

    // Read from it, checking for EINTR, as happens if called from IDL

#ifndef _WIN32
    while (((rc = (int)read(*io_data->client_socket, buf, count)) == -1) && (errno == EINTR)) {}
#else
    while (((rc = recv(*io_data->_client_socket, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
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
