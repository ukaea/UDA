#ifndef IDAM_CLIENT_IDAMCREATECONNECTION_H
#define IDAM_CLIENT_IDAMCREATECONNECTION_H

#include <clientserver/socketStructs.h>

extern SOCKETLIST client_socketlist;        // List of Data Server Sockets
extern int clientSocket;                    // The tcp/ip fd handle
extern int clientVersion;                   // Client Library Version
extern char clientUsername[STRING_LENGTH];  // Only obtain userid once

int idamCreateConnection();

#endif // IDAM_CLIENT_IDAMCREATECONNECTION_H

