#pragma once

#include "socketStructs.h"

namespace uda::client_server
{

// Initialise
void initSocketList(SOCKETLIST* socks);

// Add a New Socket to the Socket List
int addSocket(SOCKETLIST* socks, int type, int status, char* host, int port, int fh);

// Search for an Open Socket in the Socket List
int getSocket(SOCKETLIST* socks, int type, int* status, char* host, int port, int* fh);

// Search for an Open Socket in the Socket List
int getSocketRecordId(SOCKETLIST* socks, int fh);

void closeClientSockets(SOCKETLIST* socks);

void closeClientSocket(SOCKETLIST* socks, int fh);

} // namespace uda::client_server
