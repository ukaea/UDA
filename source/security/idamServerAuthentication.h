
#ifndef IDAM_IDAMSERVERAUTHENTICATION_H
#define IDAM_IDAMSERVERAUTHENTICATION_H

#ifdef SECURITYENABLED

#include "gcrypt.h"

#include "idamclientserverpublic.h"

int idamServerAuthentication(CLIENT_BLOCK *client_block, SERVER_BLOCK *server_block, unsigned short authenticationStep);

#endif

#endif // IDAM_IDAMSERVERAUTHENTICATION_H

