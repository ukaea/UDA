// Create and Manage a List of Socket Connections to External Data Servers
//
// Routines addSocket and getSocket return 0 if successfull otherwise return 1
//
//----------------------------------------------------------------------------------

#include <cstdlib>

#ifndef _WIN32
#  include <strings.h>
#  include <unistd.h>
#else
#  include <Windows.h>
#  include <winsock2.h>
#  define strcasecmp _stricmp
#  include <string.h>
#endif

#include "manageSockets.h"
#include "common/stringUtils.h"

// Initialise

void uda::client_server::init_socket_list(SOCKETLIST* socks)
{
    socks->nsocks = 0;
    socks->sockets = nullptr;
}

// Add a New Socket to the Socket List

int uda::client_server::add_socket(SOCKETLIST* socks, SocketType type, int status, const std::string& host, int port, int fh)
{
    int old_fh = -1;
    if (!get_socket(socks, type, &status, host, port, &old_fh)) { // Is an Open Socket already listed?
        if (old_fh == fh) {
            return 0;
        }
    }

    socks->sockets = (Sockets*)realloc(socks->sockets, (socks->nsocks + 1) * sizeof(SOCKETS));
    socks->sockets[socks->nsocks].type = type;
    socks->sockets[socks->nsocks].status = status;
    socks->sockets[socks->nsocks].fh = fh;
    socks->sockets[socks->nsocks].port = port;

    strcpy(socks->sockets[socks->nsocks].host, host.c_str());
    socks->sockets[socks->nsocks].tv_server_start = 0;
    socks->sockets[socks->nsocks].user_timeout = 0;

    (socks->nsocks)++;
    return 0;
}

// Search for an Open Socket in the Socket List

int uda::client_server::get_socket(SOCKETLIST* socks, SocketType type, int* status, const std::string& host, int port, int* fh)
{
    for (int i = 0; i < socks->nsocks; i++)
    {
        auto socket = socks->sockets[i];
        if (STR_IEQUALS(host.c_str(), socket.host) and socket.type == type and socket.port == port)
        {
            *status = socket.status;
            *fh = (socket.status == 1) ? socket.fh : -1;
            return 0; // Found
        }
    }
    return 1; // Failed - Need to Open a Socket/Connection
}

// Search for an Open Socket in the Socket List

int uda::client_server::get_socket_record_id(SOCKETLIST* socks, int fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh) {
            return i;
        }
    }
    return -1; // Failed - No Socket
}

void uda::client_server::close_client_socket(SOCKETLIST* socks, int fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh && socks->sockets[i].fh >= 0) {
            if (socks->sockets[i].type == SocketType::UDA) {
#ifndef _WIN32
                close(fh); // Only Genuine Sockets!
#else
                closesocket(fh);
#endif
            }
            socks->sockets[i].status = 0;
            socks->sockets[i].fh = -1;
            break;
        }
    }
}

void uda::client_server::close_client_sockets(SOCKETLIST* socks)
{
    for (int i = 0; i < socks->nsocks; i++) {
        close_client_socket(socks, socks->sockets[i].fh);
    }
    free(socks->sockets);
    init_socket_list(socks);
}
