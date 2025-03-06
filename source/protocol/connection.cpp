#include "connection.hpp"

#include "clientserver/uda_defines.h"

namespace {

constexpr int MinBlockTime = 1000;
constexpr int MaxBlockTime = 10000;

} // anon namespace

using namespace uda::client_server;

void uda::protocol::set_select_params(int fd, fd_set* rfds, struct timeval* tv, int* server_tot_block_time)
{
    FD_ZERO(rfds);    // Initialise the File Descriptor set
    FD_SET(fd, rfds); // Identify the Socket in the FD set
    tv->tv_sec = 0;
    tv->tv_usec = MinBlockTime; // minimum wait microsecs (1ms)
    *server_tot_block_time = 0;
}

void uda::protocol::update_select_params(int fd, fd_set* rfds, struct timeval* tv, int server_tot_block_time)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    if (server_tot_block_time < MaxBlock) {
        // (ms) For the First blocking period have rapid response (clientserver/uda_defines.h == 1000)
        tv->tv_sec = 0;
        tv->tv_usec = MinBlockTime; // minimum wait (1ms)
    } else {
        tv->tv_sec = 0;
        tv->tv_usec = MaxBlockTime; // maximum wait (10ms)
    }
}