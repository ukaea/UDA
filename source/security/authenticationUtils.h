#ifndef UDA_SECURITY_AUTHENTICATIONUTILS_H
#define UDA_SECURITY_AUTHENTICATIONUTILS_H

#include "udaStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int testFilePermissions(const char* object);

LIBRARY_API void initSecurityBlock(SECURITY_BLOCK* str);

#ifdef __cplusplus
}
#endif

#endif // UDA_SECURITY_AUTHENTICATIONUTILS_H
