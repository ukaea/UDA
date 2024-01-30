#ifndef UDA_SECURITY_SERVERAUTHENTICATION_H
#define UDA_SECURITY_SERVERAUTHENTICATION_H

#include "structures/genStructs.h"
#include "clientserver/udaStructs.h"

#include "security.h"

int serverAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block,
                                     LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                     AUTHENTICATION_STEP authenticationStep);

#endif // UDA_SECURITY_SERVERAUTHENTICATION_H
