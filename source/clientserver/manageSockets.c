// Create and Manage a List of Socket Connections to External Data Servers
//
// Routines addSocket and getSocket return 0 if successfull otherwise return 1
//
//----------------------------------------------------------------------------------

#include <stdlib.h>
#include <memory.h>
#include "manageSockets.h"

// Initialise

void initSocketList(SOCKETLIST *socks) {
    socks->nsocks  = 0;
    socks->sockets = NULL;
}

// Add a New Socket to the Socket List

int addSocket(SOCKETLIST *socks, int type, int status, char *host, int port, int fh) {
    int old_fh = -1;
    if(!getSocket(socks, type, &status, host, port, &old_fh)) {	// Is an Open Socket already listed?
        if(old_fh == fh) return 0;
    }

    socks->sockets = realloc(socks->sockets, (socks->nsocks + 1)*sizeof(SOCKETS));
    socks->sockets[socks->nsocks].type   = type;
    socks->sockets[socks->nsocks].status = status;
    socks->sockets[socks->nsocks].fh     = fh;
    socks->sockets[socks->nsocks].port   = port;

    strcpy(socks->sockets[socks->nsocks].host, host);
    socks->sockets[socks->nsocks].tv_server_start = 0;
    socks->sockets[socks->nsocks].user_timeout    = 0;

    (socks->nsocks)++;
    return 0;
}

// Search for an Open Socket in the Socket List

int getSocket(SOCKETLIST *socks, int type, int *status, char *host, int port, int *fh) {
    int i;
    for(i=0; i<socks->nsocks; i++) {
        if(!strcasecmp(host, socks->sockets[i].host) && socks->sockets[i].type == type && socks->sockets[i].port == port) {
            if((*status = socks->sockets[i].status) == 1) {
                *fh = socks->sockets[i].fh;
            } else {
                *fh = -1;	// Not an Open Socket
            }
            return 0;	// Found
        }
    }
    return 1; 	// Failed - Need to Open a Socket/Connection
}

// Search for an Open Socket in the Socket List

int getSocketRecordId(SOCKETLIST *socks, int fh) {
    int i;
    for(i=0; i<socks->nsocks; i++) if(socks->sockets[i].fh == fh) return(i);
    return -1; 	// Failed - No Socket
}
