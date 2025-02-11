#pragma once

#include "socket_structs.h"

#include <vector>

namespace uda::client_server
{

// Search for an Open Socket in the Socket List
Socket get_socket(const std::vector<Socket>& sockets, const std::string& host, int port);

// Search for an Open Socket in the Socket List
int get_socket_record_id(const std::vector<Socket>& sockets, int fh);

void close_client_sockets(std::vector<Socket>& sockets);

void close_client_socket(Socket& socket);

} // namespace uda::client_server
