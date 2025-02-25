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
    inline Connection(std::vector<client_server::UdaError>& error_stack)
        : error_stack_{error_stack}
        , socket_list_{}
        , host_list_{}
    {}
    inline Connection(std::vector<client_server::UdaError>& error_stack, config::Config& config)
        : Connection(error_stack)
    {
        load_config(config);
    }
    int open();
    int reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout);
    int create(XDR* client_input, XDR* client_output);
    void close_down(ClosedownType type);

    const uda::client_server::Socket& get_current_connection_data() const;
    bool current_socket_timeout() const;
    time_t get_current_socket_age() const;
    void set_maximum_socket_age(int age);

    // TODO: this returns (writeable) pointer to private member variable. intention?
    IoData io_data() { return IoData{&error_stack_, &client_socket_}; }

    void set_port(int port);
    void set_host(std::string_view host);
    void load_host_list(std::string_view config_file);

    int get_port() const;
    const std::string& get_host() const;
    bool reconnect_required() const;
    inline const ConnectionOptions get_options() const
    {
        return ConnectionOptions {max_socket_delay_, max_socket_attempts_, port_, failover_port_,
            host_, failover_host_};
    }

    void load_config(config::Config& config);


protected:
    int client_socket_ = -1;
    std::vector<client_server::UdaError>& error_stack_;
    std::vector<uda::client_server::Socket> socket_list_; // List of open sockets
    HostList host_list_ = {};
    // currently unused. could have a reset-to-config method
    // config::Config& config_;

    int port_ = DefaultPort;
    std::string host_ = DefaultHost;
    int max_socket_delay_ = DefaultMaxSocketDelay;
    int max_socket_attempts_ = DefaultMaxSocketAttempts;

    int failover_port_ = 0;
    std::string failover_host_ = {};

    mutable bool server_reconnect_ = false;
    // mutable bool server_change_socket_ = false;

    int find_socket(int fh);
    int find_socket();
    int find_socket_by_properties(std::string_view host, int port);
    void close_socket(int fh);
    void unpack_config();
    uda::client_server::Socket& get_current_socket();
};

int writeout(void* iohandle, char* buf, int count);
int readin(void* iohandle, char* buf, int count);

} // namespace client
} // namespace uda

#endif // UDA_CLIENT_CONNECTION_H
