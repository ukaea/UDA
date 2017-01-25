// Client Functions to Close Socket Connections to External Data Servers
//
//----------------------------------------------------------------------------------

#include "closeClientSockets.h"

#include <unistd.h>

#include "manageSockets.h"

void closeNamedIdamClientSocket(SOCKETLIST *socks, char *host, int port) {
    int i;
    for(i=0; i<socks->nsocks; i++) {
        if(!strcasecmp(host, socks->sockets[i].host) && socks->sockets[i].port == port) {
            if(socks->sockets[i].type == TYPE_IDAM_SERVER) {
#ifndef _WIN32
                close(socks->sockets[i].fh);		// Only Regular Sockets!
#else
                closesocket(socks->sockets[i].fh);		// Windows
#endif
            }
            socks->sockets[i].status = 0;
            socks->sockets[i].fh     = -1;
            break;
        }
    }
}

void closeIdamClientSocket(SOCKETLIST *socks, int fh) {
    int i;
    for(i=0; i<socks->nsocks; i++) {
        if(socks->sockets[i].fh == fh && socks->sockets[i].fh >= 0) {
            if(socks->sockets[i].type == TYPE_IDAM_SERVER) {
#ifndef _WIN32
                close(fh);					// Only Genuine Sockets!
#else
                closesocket(fh);
#endif
            }
            socks->sockets[i].status = 0;
            socks->sockets[i].fh     = -1;
            break;
        }
    }
}


void closeIdamClientSockets(SOCKETLIST *socks) {
    int i;
    for(i=0; i<socks->nsocks; i++) closeIdamClientSocket(socks, socks->sockets[i].fh);
    if(socks->sockets != NULL) free((void *)socks->sockets);
    initSocketList(socks);
    return;
}
