#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <vector>

#include <clientserver/socketStructs.h>
#include "udaStructs.h"
#include "export.h"

#include "closedown.hpp"

namespace uda {
namespace client {

class HostList;

struct IoData {
    int* client_socket;
};

class Connection {
public:
    explicit Connection(Environment& environment)
        : environment(environment)
        , socket_list()
    {}
    int open();
    int reconnect(XDR** client_input, XDR** client_output, time_t* tv_server_start, int* user_timeout);
    int create(XDR* client_input, XDR* client_output, const HostList& host_list);
    void close_down(ClosedownType type);
    IoData io_data() {
        return IoData{ &client_socket };
    }

private:
    int client_socket = -1;
    Environment& environment;
    std::vector<Sockets> socket_list;   // List of open sockets

    int find_socket(int fh);
    void close_socket(int fh);
};

int writeout(void* iohandle, char* buf, int count);
int readin(void* iohandle, char* buf, int count);

}
}

#endif // UDA_CLIENT_CONNECTION_H
