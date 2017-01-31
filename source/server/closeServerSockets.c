// Server Functions to Close Socket Connections to External Data Servers
//
//----------------------------------------------------------------------------------

#include "closeServerSockets.h"

#include <unistd.h>
#include <strings.h>
#include <stdlib.h>

#ifndef    NOMDSPLUSPLUGIN
#  include <mdslib.h>
#endif

#include <clientserver/manageSockets.h>

void closeNamedServerSocket(SOCKETLIST* socks, char* host, int port) {
    int i;
    for (i = 0; i < socks->nsocks; i++) {
        if (!strcasecmp(host, socks->sockets[i].host) && socks->sockets[i].port == port) {
            if (socks->sockets[i].type == TYPE_IDAM_SERVER) close(socks->sockets[i].fh);        // Only Genuine Sockets!
#ifndef    NOMDSPLUSPLUGIN
            if (socks->sockets[i].type == TYPE_MDSPLUS_SERVER) {
                SOCKET mdssocket;
                mdssocket = (SOCKET) socks->sockets[i].fh;
                MdsSetSocket(&mdssocket);
                MdsDisconnect();
            }
#endif
            socks->sockets[i].status = 0;
            socks->sockets[i].fh = -1;        // Need to call a closing function for other types of Connection, e.g. MDS+!
            break;
        }
    }
}

void closeServerSocket(SOCKETLIST* socks, int fh) {
    int i;
    for (i = 0; i < socks->nsocks; i++) {
        if (socks->sockets[i].fh == fh) {
            if (socks->sockets[i].type == TYPE_IDAM_SERVER) close(fh);                // Only Genuine Sockets!
#ifndef    NOMDSPLUSPLUGIN
            if (socks->sockets[i].type == TYPE_MDSPLUS_SERVER) {
                SOCKET mdssocket;
                mdssocket = (SOCKET) fh;
                MdsSetSocket(&mdssocket);
                MdsDisconnect();
            }
#endif
            socks->sockets[i].status = 0;
            socks->sockets[i].fh = -1;        // Need to call a closing function for other types of Connection, e.g. MDS+!
            break;
        }
    }
}

void closeServerSockets(SOCKETLIST* socks) {
    int i;
    for (i = 0; i < socks->nsocks; i++) closeServerSocket(socks, socks->sockets[i].fh);
    if (socks->sockets != NULL) free((void*) socks->sockets);
    initSocketList(socks);
    return;
}

