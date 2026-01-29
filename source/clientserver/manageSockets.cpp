// Create and Manage a List of Socket Connections to External Data Servers
//
// Routines addSocket and getSocket return 0 if successfull otherwise return 1
//
//----------------------------------------------------------------------------------

#include <cstdlib>

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

// Add a New Socket to the Socket List

int addSocket(SOCKETLIST* socks, int type, int status, char* host, int port, int fh)
{
    int old_fh = -1;
	int isock;

    if (!getSocket(socks, type, &status, host, port, &old_fh)) {    // Is an Open Socket already listed?
		isock=getSocketRecordId(socks, old_fh);
    }
	else {
    	socks->sockets = (Sockets*)realloc(socks->sockets, (socks->nsocks + 1) * sizeof(SOCKETS));
		isock=socks->nsocks;
	}
    socks->sockets[isock].type = type;
    socks->sockets[isock].status = status;
    socks->sockets[isock].fh = fh;
    socks->sockets[isock].port = port;

    strcpy(socks->sockets[isock].host, host);
    socks->sockets[isock].tv_server_start = 0;
    socks->sockets[isock].user_timeout = 0;
	socks->sockets[isock].protocol_version=-1;
	socks->sockets[isock].server_version=0;

    (socks->nsocks)++;
    return 0;
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
