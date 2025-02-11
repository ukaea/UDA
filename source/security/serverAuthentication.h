#ifndef UDA_SECURITY_SERVERAUTHENTICATION_H
#define UDA_SECURITY_SERVERAUTHENTICATION_H

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"

#include "security.h"

int serverAuthentication(CLIENT_BLOCK* client_block, ServerBlock* server_block, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, AUTHENTICATION_STEP authenticationStep);

#endif // UDA_SECURITY_SERVERAUTHENTICATION_H
