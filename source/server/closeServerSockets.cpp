#include "closeServerSockets.h"

#if defined(__GNUC__)
#  include <strings.h>
#  include <unistd.h>
#else
#  include <io.h>
#  define close _close
#endif
#include <cstdlib>

#include "clientserver/manageSockets.h"
#include "common/stringUtils.h"

using namespace uda::client_server;

void closeNamedServerSocket(SOCKETLIST* socks, char* host, int port)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (STR_IEQUALS(host, socks->sockets[i].host) && socks->sockets[i].port == port) {
            if (socks->sockets[i].type == SocketType::UDA) {
                close(socks->sockets[i].fh); // Only Genuine Sockets!
            }
            socks->sockets[i].status = 0;
            socks->sockets[i].fh = -1; // Need to call a closing function for other types of Connection, e.g. MDS+!
            break;
        }
    }
}

void uda::server::closeServerSocket(SOCKETLIST* socks, int fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh) {
            if (socks->sockets[i].type == SocketType::UDA) {
                close(fh); // Only Genuine Sockets!
            }
            socks->sockets[i].status = 0;
            socks->sockets[i].fh = -1; // Need to call a closing function for other types of Connection, e.g. MDS+!
            break;
        }
    }
}

void uda::server::closeServerSockets(SOCKETLIST* socks)
{
    for (int i = 0; i < socks->nsocks; i++) {
        closeServerSocket(socks, socks->sockets[i].fh);
    }
    if (socks->sockets != nullptr) {
        free(socks->sockets);
    }
    init_socket_list(socks);
}
