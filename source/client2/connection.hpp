#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <vector>

#include "clientserver/socket_structs.h"
#include "clientserver/uda_structs.h"

#include "closedown.hpp"
#include "host_list.hpp"

#include <string>

namespace uda::config {
class Config;
}

namespace uda {
namespace client {

constexpr auto DefaultHost = "localhost";
constexpr auto DefaultPort = 56565;

constexpr auto DefaultMaxSocketDelay = 10;
constexpr auto DefaultMaxSocketAttempts = 3;

struct IoData {
    std::vector<client_server::UdaError>* error_stack;
    int* client_socket;
};

struct ConnectionOptions
{
    int max_socket_delay = DefaultMaxSocketDelay;
    int max_socket_attempts = DefaultMaxSocketAttempts;
    int port = DefaultPort;
    int failover_port = 0;
    std::string host = DefaultHost;
    std::string failover_host {};
};

class Connection {
public:
    explicit Connection(std::vector<client_server::UdaError>& error_stack, config::Config& config)
        : error_stack_{error_stack}
        , config_{config}
        , socket_list_{}
        , _host_list{}
    {}
    explicit inline Connection(config::Config& config)
        : Connection()
    {
        load_config(config);
    }
    int open();
    int reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout);
    int create(XDR* client_input, XDR* client_output);
    void close_down(ClosedownType type);

    const uda::client_server::Sockets& get_current_connection_data() const;
    bool current_socket_timeout() const;
    time_t get_current_socket_age() const;
    void set_maximum_socket_age(int age);

    // TODO: this returns (writeable) pointer to private member variable. intention?
    IoData io_data() { return IoData{&error_stack_, &_client_socket}; }

    void set_port(int port);
    void set_host(std::string_view host);
    void load_host_list(std::string_view config_file);

    int get_port() const;
    const std::string& get_host() const;
    bool reconnect_required() const;
    inline const ConnectionOptions get_options() const
    {
        return ConnectionOptions {_max_socket_delay, _max_socket_attempts, _port, _failover_port,
            _host, _failover_host};
    }

    void load_config(config::Config& config);


protected:
    int _client_socket = -1;
    std::vector<uda::client_server::Sockets> _socket_list; // List of open sockets
    HostList _host_list = {};
    std::vector<client_server::UdaError>& error_stack_;
    config::Config& config_;
    std::vector<client_server::Socket> socket_list_; // List of open sockets

    int _port = DefaultPort;
    std::string _host = DefaultHost;
    int _max_socket_delay = DefaultMaxSocketDelay;
    int _max_socket_attempts = DefaultMaxSocketAttempts;

    int _failover_port = 0;
    std::string _failover_host = {};

    mutable bool _server_reconnect = false;
    mutable bool _server_change_socket = false;

    int find_socket(int fh);
    int find_socket();
    int find_socket_by_properties(std::string_view host, int port);
    void close_socket(int fh);
    void unpack_config();
    uda::client_server::Sockets& get_current_socket();
};

int writeout(void* iohandle, char* buf, int count);
int readin(void* iohandle, char* buf, int count);

} // namespace client
} // namespace uda

#endif // UDA_CLIENT_CONNECTION_H
