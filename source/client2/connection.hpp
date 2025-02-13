#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <vector>

#include "clientserver/socket_structs.h"
#include "clientserver/uda_structs.h"

#include "closedown.hpp"

namespace uda::config {
class Config;
}

namespace uda {
namespace client {

class HostList;

struct IoData {
    std::vector<client_server::UdaError>* error_stack;
    int* client_socket;
};

class Connection {
public:
    explicit Connection(std::vector<client_server::UdaError>& error_stack, config::Config& config)
        : error_stack_{error_stack}
        , config_{config}
        , socket_list_{}
    {}
    int open();
    int reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout);
    int create(XDR* client_input, XDR* client_output, const HostList& host_list);
    void close_down(ClosedownType type);
    IoData io_data() { return IoData{&error_stack_, &_client_socket}; }

private:
    int _client_socket = -1;
    std::vector<client_server::UdaError>& error_stack_;
    config::Config& config_;
    std::vector<client_server::Socket> socket_list_; // List of open sockets

    int find_socket(int fh);
    void close_socket(int fh);
};

int writeout(void* iohandle, char* buf, int count);
int readin(void* iohandle, char* buf, int count);

} // namespace client
} // namespace uda

#endif // UDA_CLIENT_CONNECTION_H
