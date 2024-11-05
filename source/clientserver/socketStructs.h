#pragma once

#include "udaDefines.h"

#include <rpc/rpc.h>
#include <time.h>

#include <string>

namespace uda::client_server
{

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

enum class SocketType : int {
    Unknown = 0,
    UDA = 1,
    MDSPLUS = 2,
};

//--------------------------------------------------------
// Socket Management

typedef struct Sockets {
    SocketType type;             // Type Code
    char host[MaxServer]; // Server's Host Name or IP Address
    int port;
    int status;             // Open (1) or Closed (0)
    int fh;                 // Socket to Server File Handle
    int user_timeout;       // Server's timeout value (self-destruct)
    time_t tv_server_start; // Server Startup Clock Time
    XDR* Input;             // Client Only XDR input Stream;
    XDR* Output;            // Client Only XDR Output Stream;
} SOCKETS;

typedef struct SocketList {
    int nsocks;       // Number of Sockets
    SOCKETS* sockets; // Array of Socket Management Data
} SOCKETLIST;

} // namespace uda::client_server
