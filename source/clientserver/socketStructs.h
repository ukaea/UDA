#pragma once

#ifndef UDA_CLIENTSERVER_SOCKETSTRUCTS_H
#define UDA_CLIENTSERVER_SOCKETSTRUCTS_H

#include "udaDefines.h"

#include <time.h>
#include <rpc/rpc.h>
#include "export.h"

#include <string>

struct HostData {
    std::string host_alias;
    std::string host_name;
    std::string certificate;
    std::string key;
    std::string ca_certificate;
    int port;
    bool isSSL;
};

//-------------------------------------------------------
// Socket Types

#define TYPE_UNKNOWN_SERVER 0
#define TYPE_UDA_SERVER     1
#define TYPE_MDSPLUS_SERVER 2

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
	int protocol_version;
	int server_version;
} SOCKETS;

typedef struct SocketList {
    int nsocks;             // Number of Sockets
    SOCKETS* sockets;      // Array of Socket Management Data
} SOCKETLIST;

#endif // UDA_CLIENTSERVER_SOCKETSTRUCTS_H
