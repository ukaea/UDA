// Create an IPv4 or IPv6 Socket Connection to the UDA server with a randomised time delay between connection attempts
//
//----------------------------------------------------------------
#include "client.hpp"
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

#include "clientserver/error_log.h"
#include "client_xdr_stream.hpp"
#include "closedown.hpp"
#include "clientserver/manage_sockets.h"
#include "config/config.h"
#include "exceptions.hpp"
#include "logging/logging.h"

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

void uda::client::Connection::load_config(const config::Config& config)
{
    if (!config) {
        return;
    }

    host_list_ = HostList(config);

    // TODO: what is precedence here if host is from host-list (with associated port?)
    // TODO: should connection class always have reference to host list?
    auto port = config.get("connection.port").as_or_default<int32_t>(DefaultPort);
    if (port != port_) {
        set_port(port);
    }
    auto host = config.get("connection.host").as_or_default<std::string>(DefaultHost);
    if (host != host_) {
        set_host(host);
    }

    auto max_socket_delay = config.get("connection.max_socket_delay").as_or_default<int>(DefaultMaxSocketDelay);
    if (max_socket_delay != max_socket_delay_)
    {
        max_socket_delay_ = max_socket_delay;
        server_reconnect_ = true;
    }
    auto max_socket_attempts = config.get("connection.max_socket_attempts").as_or_default<int>(DefaultMaxSocketAttempts);
    if (max_socket_attempts != max_socket_attempts_)
    {
        max_socket_attempts_ = max_socket_delay;
        server_reconnect_ = true;
    }

    // TODO: currently no host_list_ searching for failover connection details.
    // TODO: how to determine if currently connected to failover server and signal reconnect on update?
    fail_over_port_ = config.get("connection.failover_port").as_or_default<int32_t>(0);
    fail_over_host_ = config.get("connection.failover_host").as_or_default<std::string>(""s);
}

void uda::client::Connection::load_host_list(std::string_view config_file)
{
    host_list_ = HostList(config_file);
}

int uda::client::Connection::open()
{
    return client_socket_ != -1;
}

int uda::client::Connection::find_socket(int fh)
{
    int i = 0;
    for (const auto& socket : socket_list_) {
        if (socket.fh == fh) {
            return i;
        }
        ++i;
    }
    return -1;
}

int uda::client::Connection::find_socket()
{
    return find_socket(client_socket_);
}

int uda::client::Connection::find_socket_by_properties(std::string_view host, int port)
{
    int i = 0;
    for (const auto& socket : socket_list_) {
        if (std::string(socket.host) == host and socket.port == port) {
            return i;
        }
        ++i;
    }
    return -1;
}

const uda::client_server::Socket& uda::client::Connection::get_current_connection_data() const
{
    if (client_socket_ == -1) {
        throw uda::exceptions::ClientError("No open socket connection");
    }
    for (const auto& socket : socket_list_) {
        if (socket.fh == client_socket_) {
            return socket;
        }
    }
    throw uda::exceptions::ClientError(
        "Client socket file handle not found in socket list. Connection state is corrupted");
}

uda::client_server::Socket& uda::client::Connection::get_current_socket()
{
    if (client_socket_ == -1) {
        throw uda::exceptions::ClientError("No open socket connection");
    }

    for (auto& socket : socket_list_) {
        if (socket.fh == client_socket_) {
            return socket;
        }
    }
    throw uda::exceptions::ClientError(
        "Client socket file handle not found in socket list. Connection state is corrupted");
}

// TODO: is it worth using chrono to improve precision of this?
time_t uda::client::Connection::get_current_socket_age() const
{
    if (client_socket_ == -1) {
        return 0;
    }

    // note precision of ctime::time is 1s only
    const auto& socket = get_current_connection_data();
    time_t current_time = time(nullptr);
    return (current_time - socket.tv_server_start);
}

bool uda::client::Connection::current_socket_timeout() const
{
    if (client_socket_ == -1) {
        return true;
    }

    const auto& socket = get_current_connection_data();
    time_t age = get_current_socket_age();
    return age >= socket.user_timeout;
}

void uda::client::Connection::set_maximum_socket_age(int age)
{
    if (client_socket_ == -1) {
        return;
    }
    auto& socket = get_current_socket();
    socket.user_timeout = age;
}

bool uda::client::Connection::maybe_reuse_existing_socket()
{
    int candidate_socket_id = find_socket_by_properties(host_, port_);
    if (candidate_socket_id == -1 or !socket_list_[candidate_socket_id].open) {
        client_socket_ = -1;
        return false;
    }
    client_socket_ = socket_list_[candidate_socket_id].fh;
    if (current_socket_timeout()) {
        close_down(ClosedownType::CLOSE_SOCKETS);
        client_socket_ = -1;
        return false;
    }
    return true;
}

//TODO: replace with three functions: (i) swap to another existing connection. (ii) create new connection. (iii) set and get socket details
int uda::client::Connection::reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start,
                                       int* user_timeout)
{
    // TODO: this doesn't look right... this invalidates the client_socket handle but doesn't
    // call e.g. ::create to actually create the new connection and register the socket
    // relies on a later call to ::open() to check client_socket_!=-1
    // -- Basically this signals to create a new connection later

    // TODO: the in/out args feels a bit like side-effects? I don't like this signature.
    // is the intention to establish a new connection, or to extract the connection details?

    int err = 0;

    // Save current client and server timer settings, Socket and XDR handles

    time_t tv_server_start0 = *tv_server_start;
    int user_timeout0 = *user_timeout;
    int client_socket0 = client_socket_;
    XDR* client_input0 = *client_input;
    XDR* client_output0 = *client_output;

    // Identify the current Socket connection in the Socket List
    int socket_id = find_socket();

    // TODO: missing the change_socket logic here, which says (in client1) that if another (live) connection exists
    //  just change to that one
    int candidate_socket_id = find_socket_by_properties(host_, port_);
    if (candidate_socket_id != -1 and socket_list_[candidate_socket_id].open) {
        client_socket_ = socket_list_[candidate_socket_id].fh;

        // replace with previous timer settings and XDR handles
        *tv_server_start = socket_list_[candidate_socket_id].tv_server_start;
        *user_timeout = socket_list_[candidate_socket_id].user_timeout;
        client_socket_ = socket_list_[candidate_socket_id].fh;
        *client_input = socket_list_[candidate_socket_id].Input;
        *client_output = socket_list_[candidate_socket_id].Output;

        port_ = socket_list_[candidate_socket_id].port;
        host_ = std::string{socket_list_[candidate_socket_id].host};
    }

    // Instance a new server if the Client has changed the host and/or port number

    else if (server_reconnect_) {
        time(tv_server_start); // Start a New Server AGE timer
        client_socket_ = -1;   // Flags no Socket is open
        // server_change_socket_ = false;
    }

    // Client manages connections through the Socket id and specifies which running server to connect to

    // save Previous data if a previous socket existed
    if (socket_id >= 0) {
        socket_list_[socket_id].tv_server_start = tv_server_start0;
        socket_list_[socket_id].user_timeout = user_timeout0;
        socket_list_[socket_id].fh = client_socket0;
        socket_list_[socket_id].Input = client_input0;
        socket_list_[socket_id].Output = client_output0;
    }

    return err;
}

void localhost_info(int* ai_family)
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

void set_hints(struct addrinfo* hints, const char* host_name)
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
        localhost_info(&hints->ai_family);
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
        bool is_numeric = true;
        for (const auto& token : list) {
            size_t lstr = token.size();
            for (size_t j = 0; j < lstr; j++) {
                is_numeric &= (bool)std::isdigit(token[j]);
            }
        }

        if (is_numeric) {
            hints->ai_flags = hints->ai_flags | AI_NUMERICHOST;
        }
    }
}

bool uda::client::Connection::reconnect_required() const
{
    return server_reconnect_;
}

int uda::client::Connection::get_port() const
{
    return port_;
}

const std::string& uda::client::Connection::get_host() const
{
    return host_;
}

void uda::client::Connection::set_port(int port)
{
    if (port == port_) {
        return;
    }
    port_ = port;
    server_reconnect_ = true;
}

void uda::client::Connection::set_host(std::string_view host)
{
    if (host == host_) {
        return;
    }
    auto host_list_record = host_list_.find_by_alias(host);
    if (host_list_record != nullptr) {
        if (host_list_record->host_name != host_) {
            host_ = host_list_record->host_name;
            server_reconnect_ = true;
        }
        int port = host_list_record->port;
        if (port > 0 && port_ != port) {
            port_ = port;
            server_reconnect_ = true;
        }
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
        put_client_ssl_protocol(host_list_record->isSSL);
#endif
    } else if ((host_list_record = host_list_.find_by_name(host)) != nullptr) {
        host_ = host;
        server_reconnect_ = true;
        int port = host_list_record->port;
        if (port > 0 && port_ != port) {
            // Replace if found and different
            port_ = port;
        }
    } else {
        // Does the host name contain the SSL protocol prefix? If so strip this off
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
        if (boost::starts_with(host, "SSL://")) {
            auto new_host = host.substr(6);
            if (new_host == host_) {
                host_ = new_host;
            }
            put_client_ssl_protocol(1);
        } else {
            host_ = host;
        }
#else
        host_ = host;
#endif
        server_reconnect_ = true;
    }
}

// note this passes ownership of the XDR stream pointers
void uda::client::Connection::register_xdr_streams(XDR* client_input, XDR* client_output)
{
    //throws if there is no current connection.
    auto& socket = get_current_socket();
    socket.Input = client_input;
    socket.Output = client_output;
}

void uda::client::Connection::register_new_xdr_streams()
{
    io_data_list_.emplace_back(io_data());
    auto [client_input, client_output] = create_xdr_stream(&io_data_list_.back());
    if (!client_input or !client_output) {
        throw uda::exceptions::ClientError("failed to open new XDR streams");
    }

    register_xdr_streams(client_input.release(), client_output.release());
}

std::pair<XDR*, XDR*> uda::client::Connection::get_socket_xdr_streams() const
{
    const auto& socket = get_current_connection_data();
    return std::make_pair(socket.Input, socket.Output);
}

int uda::client::Connection::create()
{
    int window_size = DBReadBlockSize; // 128K
    int rc;

    //TODO: check this is always intended behaviour
    if (client_socket_ >= 0) {
        // Check Already Opened?
        return 0;
    }

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    put_client_ssl_socket(client_socket_);
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

    char service_port[PORT_STRING];
    snprintf(service_port, PORT_STRING, "%d", port_);

    // Resolve the Host and the IP protocol to be used (Hints not used)

    struct addrinfo* result = nullptr;
    struct addrinfo hints = {0};
    set_hints(&hints, host_.c_str());

    errno = 0;
    if ((rc = getaddrinfo(host_.c_str(), service_port, &hints, &result)) != 0 || (errno != 0 && errno != ESRCH)) {
        add_error(error_stack_, ErrorType::System, __func__, rc, (char*)gai_strerror(rc));
        if (rc == EAI_SYSTEM || errno != 0) {
            add_error(error_stack_, ErrorType::System, __func__, errno, "");
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
    client_socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (client_socket_ < 0 || errno != 0) {
        if (errno != 0) {
            add_error(error_stack_, ErrorType::System, __func__, errno, "");
        } else {
            add_error(error_stack_, ErrorType::Code, __func__, -1, "Problem Opening Socket");
        }
        if (client_socket_ != -1) {
#ifndef _WIN32
            ::close(client_socket_);
#else
            ::closesocket(client_socket_);
#endif
        }
        client_socket_ = -1;
        freeaddrinfo(result);
        return -1;
    }

    // Connect to server

    errno = 0;
    while ((rc = connect(client_socket_, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

    if (rc < 0 || (errno != 0 && errno != EINTR)) {

        // Try again for a maximum number of tries with a random time delay between attempts

        int ps;
        ps = getpid();
        srand((unsigned int)ps); // Seed the random number generator with the process id
        unsigned int delay = max_socket_delay_ > 0 ? (unsigned int)(rand() % max_socket_delay_) : 0; // random delay
        sleep(delay);
        errno = 0;                                       // wait period
        for (int i = 0; i < max_socket_attempts_; i++) { // try again
            while ((rc = connect(client_socket_, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}

            if (rc == 0 && errno == 0) {
                break;
            }

            delay = max_socket_delay_ > 0 ? (unsigned int)(rand() % max_socket_delay_) : 0;
            sleep(delay); // wait period
        }

        if (rc != 0 || errno != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Connect errno = {}", errno);
            UDA_LOG(UDA_LOG_DEBUG, "Connect rc = {}", rc);
            UDA_LOG(UDA_LOG_DEBUG, "Unable to connect to primary host: {} on port {}", host_, service_port);
        }

        // Abandon the principal Host - attempt to connect to the secondary host
        if (rc < 0 && !fail_over_host_.empty() && host_ != fail_over_host_) {

            freeaddrinfo(result);
            result = nullptr;
#ifndef _WIN32
            ::close(client_socket_);
#else
            ::closesocket(client_socket_);
#endif
            client_socket_ = -1;
            // TODO: check this is correct intent: principal host becomes the failover host too
            host_ = fail_over_host_;

            // Check if the host_name is an alias for an IP address or name in the client configuration - replace if
            // found

            snprintf(service_port, PORT_STRING, "%d", fail_over_port_);

            //             // Does the host name contain the SSL protocol prefix? If so strip this off
            //
            // #if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
            //             if (boost::starts_with(host_name, "SSL://")) {
            //                 // Should be stripped already if via the HOST client configuration file
            //                 fail_over_host_ = host_name.substr(6);
            //                 putUdaClientSSLProtocol(1);
            //             } else {
            //                 if (host != nullptr && host->isSSL) {
            //                     putUdaClientSSLProtocol(1);
            //                 } else {
            //                     putUdaClientSSLProtocol(0);
            //                 }
            //             }
            // #endif

            // Resolve the Host and the IP protocol to be used (Hints not used)

            set_hints(&hints, host_.c_str());

            errno = 0;
            if ((rc = getaddrinfo(host_.c_str(), service_port, &hints, &result)) != 0 || errno != 0) {
                add_error(error_stack_, ErrorType::System, __func__, rc, (char*)gai_strerror(rc));
                if (rc == EAI_SYSTEM || errno != 0) {
                    add_error(error_stack_, ErrorType::System, __func__, errno, "");
                }
                if (result) {
                    freeaddrinfo(result);
                }
                return -1;
            }
            errno = 0;
            client_socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

            if (client_socket_ < 0 || errno != 0) {
                if (errno != 0) {
                    add_error(error_stack_, ErrorType::System, __func__, errno, "");
                } else {
                    add_error(error_stack_, ErrorType::Code, __func__, -1, "Problem Opening Socket");
                }
                if (client_socket_ != -1) {
#ifndef _WIN32
                    ::close(client_socket_);
#else
                    ::closesocket(client_socket_);
#endif
                }
                client_socket_ = -1;
                freeaddrinfo(result);
                return -1;
            }

            for (int i = 0; i < max_socket_attempts_; i++) {
                while ((rc = connect(client_socket_, result->ai_addr, result->ai_addrlen)) && errno == EINTR) {}
                if (rc == 0) {
                    std::swap(fail_over_port_, port_);
                    std::swap(fail_over_host_, host_);
                    break;
                }
                delay = max_socket_delay_ > 0 ? (unsigned int)(rand() % max_socket_delay_) : 0;
                sleep(delay); // wait period
            }
        }

        if (rc < 0) {
            if (errno != 0) {
                add_error(error_stack_, ErrorType::System, __func__, errno, "");
            } else {
                add_error(error_stack_, ErrorType::Code, __func__, -1, "Unable to Connect to Server Stream Socket");
            }
            if (client_socket_ != -1)
#ifndef _WIN32
            {
                ::close(client_socket_);
            }
#else
                closesocket(client_socket_);
#endif
            client_socket_ = -1;
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

    setsockopt(client_socket_, SOL_SOCKET, SO_SNDBUF, (char*)&window_size, sizeof(window_size));

    setsockopt(client_socket_, SOL_SOCKET, SO_RCVBUF, (char*)&window_size, sizeof(window_size));

    // Other Socket Options

    int on = 1;
    if (setsockopt(client_socket_, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        add_error(error_stack_, ErrorType::Code, __func__, -1, "Error Setting KEEPALIVE on Socket");
        ::close(client_socket_);
        client_socket_ = -1;
        return -1;
    }
    on = 1;
    if (setsockopt(client_socket_, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)) < 0) {
        add_error(error_stack_, ErrorType::Code, __func__, -1, "Error Setting NODELAY on Socket");
        ::close(client_socket_);
        client_socket_ = -1;
        return -1;
    }

    // Add New Socket to the Socket's List

    Socket socket = {};

    socket.open = true;
    socket.fh = client_socket_;
    socket.port = port_;
    socket.host = host_;
    socket.tv_server_start = time(nullptr);
    socket.user_timeout = DefaultTimeout;
    socket.Input = nullptr;
    socket.Output = nullptr;

    socket_list_.push_back(socket);

    // Write the socket number to the SSL functions

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    put_client_ssl_socket(client_socket_);
#endif

    startup_state = true;

    return 0;
}

// TODO: should the closed socket also be popped from the socket_list_?
//  no records ever deleted currently
void uda::client::Connection::close_socket(int fh)
{
    for (auto& socket : socket_list_) {
        if (socket.open && socket.fh == fh && socket.fh >= 0) {
#ifndef _WIN32
            ::close(fh); // Only Genuine Socket!
#else
            ::closesocket(fh);
#endif
            socket.open = false;
            socket.fh = -1;
            close_xdr_stream(socket.Input);
            close_xdr_stream(socket.Output);
            break;
        }
    }
}

void uda::client::Connection::close_down(ClosedownType type)
{
    if (client_socket_ >= 0 && type != ClosedownType::CLOSE_ALL) {
        close_socket(client_socket_);
    } else if (type == ClosedownType::CLOSE_ALL) {
        for (const auto& socket : socket_list_) {
            close_socket(socket.fh);
        }
    }

    client_socket_ = -1;
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
    size_t bytes_sent = 0;

    fd_set wfds;
    struct timeval tv = {};

    auto* io_data = static_cast<IoData*>(iohandle);
    auto* error_stack = io_data->error_stack;

    update_select_params(*io_data->client_socket, &wfds, &tv);

    errno = 0;

    while (select(*io_data->client_socket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                UDA_LOG(UDA_LOG_DEBUG, "ECONNRESET error!");
                add_error(*error_stack, ErrorType::Code, __func__, -2,
                          "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    UDA_LOG(UDA_LOG_DEBUG, "ENETUNREACH error!");
                    add_error(*error_stack, ErrorType::Code, __func__, -3, "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    UDA_LOG(UDA_LOG_DEBUG, "ECONNREFUSED error!");
                    add_error(*error_stack, ErrorType::Code, __func__, -4, "Server Unavailable: ECONNREFUSED");
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
        add_error(*error_stack, ErrorType::Code, __func__, -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }
#endif

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (bytes_sent < static_cast<unsigned int>(count)) {
#ifndef _WIN32
        while (((rc = static_cast<int>(write(*io_data->client_socket, buf, count))) == -1) && (errno == EINTR)) {}
#else
        while (((rc = send(*io_data->client_socket_, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
#endif
        bytes_sent += rc;
        buf += rc;
    }

    // Restore the original SIGPIPE handler set by the application

#ifndef _WIN32
    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        add_error(*error_stack, ErrorType::Code, __func__, -1, "Error attempting to restore SIG_PIPE handler");
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

    auto* io_data = static_cast<IoData*>(iohandle);
    auto* error_stack = io_data->error_stack;

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
    while (((rc = recv(*io_data->client_socket_, buf, count, 0)) == SOCKET_ERROR) && (errno == EINTR)) {}
#endif

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
        if (errno != 0 && errno != EINTR) {
            add_error(*error_stack, ErrorType::System, __func__, rc, "");
        }
        add_error(*error_stack, ErrorType::Code, __func__, rc, "No Data waiting at Socket when Data Expected!");
    }

    return rc;
}
