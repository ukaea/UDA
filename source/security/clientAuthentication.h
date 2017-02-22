#ifndef UDA_SECURITY_CLIENTAUTHENTICATION_H
#define UDA_SECURITY_CLIENTAUTHENTICATION_H

#include <clientserver/udaStructs.h>

int clientAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, unsigned short authenticationStep);

#endif // UDA_SECURITY_CLIENTAUTHENTICATION_H
