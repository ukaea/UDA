#ifndef UDA_SECURITY_AUTHENTICATIONUTILS_H
#define UDA_SECURITY_AUTHENTICATIONUTILS_H

#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int testFilePermissions(const char* object);

LIBRARY_API void initSecurityBlock(SECURITY_BLOCK* str);

#ifdef __cplusplus
}
#endif

#endif // UDA_SECURITY_AUTHENTICATIONUTILS_H
