#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <vector>

#include "clientserver/socket_structs.h"
#include "clientserver/uda_structs.h"

#include "closedown.hpp"
#include "host_list.hpp"

#include <string>

using XDR = struct __rpc_xdr;

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
    Connection(std::vector<client_server::UdaError>& error_stack)
        : _error_stack{error_stack}
    {}
    Connection(std::vector<client_server::UdaError>& error_stack, config::Config& config)
        : Connection(error_stack)
    {
        load_config(config);
    }
    Connection& operator=(Connection&& other) noexcept {
        std::swap(_client_socket, other._client_socket);
        std::swap(_host_list, other._host_list);
        std::swap(_error_stack, other._error_stack);
        std::swap(_socket_list, other._socket_list);
        std::swap(_port, other._port);
        std::swap(_host, other._host);
        std::swap(_max_socket_delay, other._max_socket_delay);
        std::swap(_max_socket_attempts, other._max_socket_attempts);
        std::swap(_fail_over_port, other._fail_over_port);
        std::swap(_fail_over_port, other._fail_over_port);
        std::swap(_server_reconnect, other._server_reconnect);
        // std::swap(server_change_socket_, other.server_change_socket_);
        return *this;
    }
    bool open() const;
    int reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout);
    int create();
    void close_down(ClosedownType type);
    // void ensure_connection();

    const uda::client_server::Socket& get_current_connection_data() const;
    bool current_socket_timeout() const;
    time_t get_current_socket_age() const;
    void set_maximum_socket_age(int age);

    void register_xdr_streams(XDR* client_input, XDR* client_output);
    void register_new_xdr_streams();
    std::pair<XDR*, XDR*> get_socket_xdr_streams() const;
    bool maybe_reuse_existing_socket();
    // find_or_create_socket()

    // TODO: this returns (writeable) pointer to private member variable. intention?
    IoData io_data() { return IoData{&_error_stack, &_client_socket}; }

    void set_port(int port);
    void set_host(std::string_view host);
    void load_host_list(std::string_view config_file);

    int get_port() const;
    const std::string& get_host() const;
    bool reconnect_required() const;
    ConnectionOptions get_options() const
    {
        return ConnectionOptions {_max_socket_delay, _max_socket_attempts, _port, _fail_over_port,
            _host, _fail_over_host};
    }

    void load_config(const config::Config& config);

    bool startup_state = true;

protected:
    int _client_socket = -1;
    HostList _host_list = {};
    std::vector<client_server::UdaError>& _error_stack;
    std::vector<client_server::Socket> _socket_list; // List of open sockets

    std::vector<IoData> _io_data_list;

    int _port = DefaultPort;
    std::string _host = DefaultHost;
    int _max_socket_delay = DefaultMaxSocketDelay;
    int _max_socket_attempts = DefaultMaxSocketAttempts;

    int _fail_over_port = 0;
    std::string _fail_over_host = {};

    mutable bool _server_reconnect = false;
   // mutable bool server_change_socket_ = false;

    int find_socket(int fh) const;
    int find_socket() const;
    int find_socket_by_properties(std::string_view host, int port) const;
    void close_socket(int fh);
    void unpack_config();
    client_server::Socket& get_current_socket();
};

int writeout(void* iohandle, char* buf, int count);
int readin(void* iohandle, char* buf, int count);

} // namespace client
} // namespace uda

#endif // UDA_CLIENT_CONNECTION_H
