#ifndef UDA_SECURITY_CLIENTAUTHENTICATION_H
#define UDA_SECURITY_CLIENTAUTHENTICATION_H

#include "export.h"
#include "genStructs.h"
#include "clientserver/udaStructs.h"

#include "security.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int clientAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block,
                                     LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                     AUTHENTICATION_STEP authenticationStep);

#ifdef __cplusplus
}
#endif

#endif // UDA_SECURITY_CLIENTAUTHENTICATION_H
