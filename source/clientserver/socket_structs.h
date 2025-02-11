#pragma once

#include <time.h>
#include <string>

using XDR = struct __rpc_xdr;

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

//--------------------------------------------------------
// Socket Management

struct Socket {
    std::string host;       // Server's Host Name or IP Address
    int port = -1;
    bool open = false;      // Open (1) or Closed (0)
    int fh = -1;            // Socket to Server File Handle
    int user_timeout;       // Server's timeout value (self-destruct)
    time_t tv_server_start; // Server Startup Clock Time
    XDR* Input;             // Client Only XDR input Stream;
    XDR* Output;            // Client Only XDR Output Stream;
};

} // namespace uda::client_server
