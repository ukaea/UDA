#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <clientserver/socketStructs.h>
#include <clientserver/udaStructs.h>

extern int clientVersion;                   // Client Library Version
extern char clientUsername[STRING_LENGTH];  // Only obtain userid once

int connectionOpen();
int reconnect(ENVIRONMENT* environment);
int createConnection();
void closeConnection(int type);

int clientWriteout(void* iohandle, char* buf, int count);
int clientReadin(void* iohandle, char* buf, int count);

#endif // UDA_CLIENT_CONNECTION_H