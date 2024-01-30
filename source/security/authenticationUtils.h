#ifndef UDA_SECURITY_AUTHENTICATIONUTILS_H
#define UDA_SECURITY_AUTHENTICATIONUTILS_H

#include "clientserver/udaStructs.h"

int testFilePermissions(const char* object);

void initSecurityBlock(SECURITY_BLOCK* str);

#endif // UDA_SECURITY_AUTHENTICATIONUTILS_H
