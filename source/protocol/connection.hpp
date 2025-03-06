#pragma once

#include <vector>

#include "clientserver/uda_structs.h"

namespace uda::protocol {

struct IoData {
    int server_socket;
    int* server_tot_block_time;
    int* server_timeout;
    std::vector<client_server::UdaError>* error_stack;
};

void set_select_params(int fd, fd_set* rfds, timeval* tv, int* server_tot_block_time);

void update_select_params(int fd, fd_set* rfds, timeval* tv, int server_tot_block_time);

} // namespace uda::protocol