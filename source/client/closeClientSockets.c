//! $LastChangedRevision: 226 $
//! $LastChangedDate: 2011-02-15 10:28:26 +0000 (Tue, 15 Feb 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/closeClientSockets.c $

// Client Functions to Close Socket Connections to External Data Servers
//
// Change History:
//
// v0.00	DGMuir 16Jan2007	Original Code
// v0.01	DGMuir 21Mar2007	server_socketlist pointer argument added
//					functions renamed ...ClientSocket
//					closeClientSocket added
// 14Nov2007	dgm	Modified closeIdamClientSocket to avoid closing fh = -1
// 31Jan2011	dgm	Windows sockets implementation
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
