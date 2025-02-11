#include "manage_sockets.h"

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#  include <winsock2.h>
#endif

#include <boost/algorithm/string.hpp>

#include "common/string_utils.h"

// Search for an Open Socket in the Socket List

uda::client_server::Socket uda::client_server::get_socket(const std::vector<Socket>& sockets, const std::string& host, const int port)
{
    for (const auto& sock : sockets) {
        if (boost::iequals(sock.host, host) && sock.port == port) {
            return sock;
        }
    }
    return {};
}

// Search for an Open Socket in the Socket List

int uda::client_server::get_socket_record_id(const std::vector<Socket>& sockets, const int fh)
{
    for (size_t i = 0; i < sockets.size(); i++) {
        if (sockets[i].fh == fh) {
            return static_cast<int>(i);
        }
    }
    return -1; // Failed - No Socket
}

void uda::client_server::close_client_socket(Socket& socket)
{
    if (!socket.open) {
        return;
    }
#ifndef _WIN32
    close(socket.fh); // Only Genuine Sockets!
#else
    closesocket(socket.fh);
#endif
    socket.open = false;
    socket.fh = -1;
}

void uda::client_server::close_client_sockets(std::vector<Socket>& sockets)
{
    for (auto& socket : sockets) {
        close_client_socket(socket);
    }
    sockets.clear();
}
