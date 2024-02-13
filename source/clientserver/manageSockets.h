#pragma once

#include "socketStructs.h"

namespace uda::client_server
{

// Initialise
void init_socket_list(SOCKETLIST* socks);

// Add a New Socket to the Socket List
int add_socket(SOCKETLIST* socks, int type, int status, char* host, int port, int fh);

// Search for an Open Socket in the Socket List
int get_socket(SOCKETLIST* socks, int type, int* status, char* host, int port, int* fh);

// Search for an Open Socket in the Socket List
int get_socket_record_id(SOCKETLIST* socks, int fh);

void close_client_sockets(SOCKETLIST* socks);

void close_client_socket(SOCKETLIST* socks, int fh);

} // namespace uda::client_server
