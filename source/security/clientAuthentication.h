#ifndef UDA_SECURITY_CLIENTAUTHENTICATION_H
#define UDA_SECURITY_CLIENTAUTHENTICATION_H

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"

#include "security.h"

int clientAuthentication(CLIENT_BLOCK* client_block, ServerBlock* server_block, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, AUTHENTICATION_STEP authenticationStep);

#endif // UDA_SECURITY_CLIENTAUTHENTICATION_H
