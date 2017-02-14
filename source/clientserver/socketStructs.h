#ifndef UDA_CLIENTSERVER_SOCKETSTRUCTS_H
#define UDA_CLIENTSERVER_SOCKETSTRUCTS_H

#include "idamDefines.h"

#include <time.h>
#include <rpc/rpc.h>

//--------------------------------------------------------
// Socket Management

typedef struct Sockets {
    int type;               // Type Code (e.g.,1=>IDAM;2=>MDS+);
    char host[MAXSERVER];   // Server's Host Name or IP Address
    int port;
    int status;             // Open (1) or Closed (0)
    int fh;                 // Socket to Server File Handle
    int user_timeout;       // Server's timeout value (self-destruct)
    time_t tv_server_start; // Server Startup Clock Time
    XDR* Input;             // Client Only XDR input Stream;
    XDR* Output;            // Client Only XDR Output Stream;
} SOCKETS;

typedef struct SocketList {
    int nsocks;             // Number of Sockets
    SOCKETS* sockets;      // Array of Socket Management Data
} SOCKETLIST;

#endif // UDA_CLIENTSERVER_SOCKETSTRUCTS_H
