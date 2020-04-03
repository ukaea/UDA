#ifndef UDA_SECURITY_CLIENTAUTHENTICATION_H
#define UDA_SECURITY_CLIENTAUTHENTICATION_H

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#include "security.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int clientAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, AUTHENTICATION_STEP authenticationStep);

#ifdef __cplusplus
}
#endif

#endif // UDA_SECURITY_CLIENTAUTHENTICATION_H
