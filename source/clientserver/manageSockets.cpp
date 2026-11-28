// Create and Manage a List of Socket Connections to External Data Servers
//
// Routines addSocket and getSocket return 0 if successfull otherwise return 1
//
//----------------------------------------------------------------------------------

#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#  include <unistd.h>
#  include <strings.h>
#else
#  include <winsock2.h>
#  include <Windows.h>
#  define strcasecmp _stricmp
#  include <string.h>
#endif

#include "manageSockets.h"
#include "stringUtils.h"

// Initialise

void initSocketList(SOCKETLIST* socks)
{
    socks->nsocks = 0;
    socks->sockets = nullptr;
}

namespace {

bool matchesEndpoint(const SOCKETS* sock, int type, const char* host, int port)
{
    return sock->type == type && sock->port == port && STR_IEQUALS(host, sock->host);
}

bool isOpen(const SOCKETS* sock)
{
    return sock->status == 1 && sock->fh >= 0;
}

} // namespace

// Add a New Socket to the Socket List
//
// A closed record for the same endpoint is reused, otherwise a record is appended. Registering the same open
// descriptor for the same endpoint again is a no-op. Returns 0 on success, 1 on failure (list left unchanged).

int addSocket(SOCKETLIST* socks, int type, int status, char* host, int port, int fh)
{
    if (socks == nullptr || host == nullptr || strlen(host) >= MAXSERVER) {
        return 1;
    }

    int reuse = -1;

    for (int i = 0; i < socks->nsocks; i++) {
        SOCKETS* sock = &socks->sockets[i];
        if (fh >= 0 && sock->fh == fh && isOpen(sock)) {
            if (matchesEndpoint(sock, type, host, port)) {
                return 0;    // Already registered
            }
            // The descriptor was closed outside the list and reused by the OS: retire the stale record
            // without closing the descriptor, which now belongs to the new connection.
            sock->status = 0;
            sock->fh = -1;
        }
        if (reuse < 0 && !isOpen(sock) && matchesEndpoint(sock, type, host, port)) {
            reuse = i;
        }
    }

    // A record of a different open descriptor to the same endpoint stays in the list until the client closes it.

    if (reuse < 0) {
        auto sockets = (SOCKETS*)realloc(socks->sockets, (socks->nsocks + 1) * sizeof(SOCKETS));
        if (sockets == nullptr) {
            return 1;
        }
        socks->sockets = sockets;
    }

    SOCKETS* sock = &socks->sockets[reuse < 0 ? socks->nsocks : reuse];
    memset(sock, 0, sizeof(SOCKETS));
    sock->type = type;
    sock->status = status;
    sock->fh = fh;
    sock->port = port;
    strcpy(sock->host, host);    // Length checked above
    sock->tv_server_start = 0;
    sock->user_timeout = 0;
    sock->Input = nullptr;
    sock->Output = nullptr;
    sock->protocol_version = -1;
    sock->server_version = 0;

    if (reuse < 0) {
        socks->nsocks++;
    }

    return 0;
}

// Search for an Open Socket to an endpoint, skipping any closed records. Returns the record id or -1.

int findOpenSocketRecordId(SOCKETLIST* socks, int type, const char* host, int port)
{
    if (socks == nullptr || host == nullptr) {
        return -1;
    }
    for (int i = 0; i < socks->nsocks; i++) {
        if (isOpen(&socks->sockets[i]) && matchesEndpoint(&socks->sockets[i], type, host, port)) {
            return i;
        }
    }
    return -1;
}

// Search for an Open Socket in the Socket List

int getSocket(SOCKETLIST* socks, int type, int* status, char* host, int port, int* fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (STR_IEQUALS(host, socks->sockets[i].host) && socks->sockets[i].type == type &&
            socks->sockets[i].port == port) {
            if ((*status = socks->sockets[i].status) == 1) {
                *fh = socks->sockets[i].fh;
            } else {
                *fh = -1;    // Not an Open Socket
            }
            return 0;    // Found
        }
    }
    return 1;    // Failed - Need to Open a Socket/Connection
}

// Search for an Open Socket in the Socket List

int getSocketRecordId(SOCKETLIST* socks, int fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh) {
            return i;
        }
    }
    return -1;    // Failed - No Socket
}

void closeClientSocket(SOCKETLIST* socks, int fh)
{
    for (int i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh && socks->sockets[i].fh >= 0) {
            if (socks->sockets[i].type == TYPE_UDA_SERVER) {
#ifndef _WIN32
                close(fh);                    // Only Genuine Sockets!
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

void closeClientSockets(SOCKETLIST* socks)
{
    for (int i = 0; i < socks->nsocks; i++) {
        closeClientSocket(socks, socks->sockets[i].fh);
    }
    free(socks->sockets);
    initSocketList(socks);
}
