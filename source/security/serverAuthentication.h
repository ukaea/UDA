#ifndef UDA_SECURITY_SERVERAUTHENTICATION_H
#define UDA_SECURITY_SERVERAUTHENTICATION_H

#include <clientserver/udaStructs.h>

int serverAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, unsigned short authenticationStep);

#endif // UDA_SECURITY_SERVERAUTHENTICATION_H

