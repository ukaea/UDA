#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void udaClientFreeHostList(void);
LIBRARY_API int udaClientFindHostByAlias(const char* alias);
LIBRARY_API int udaClientFindHostByName(const char* name);
LIBRARY_API void udaClientInitHostList(void);
LIBRARY_API const char* udaClientGetHostName(int id);
LIBRARY_API int udaClientGetHostPort(int id);
LIBRARY_API void udaClientPutHostNameId(int id);

#if defined(SSLAUTHENTICATION)
LIBRARY_API const char* udaClientGetHostCertificatePath(int id);
LIBRARY_API const char* udaClientGetHostAlias(int id);
LIBRARY_API const char* udaClientGetHostKeyPath(int id);
LIBRARY_API const char* udaClientGetHostCAPath(int id);
LIBRARY_API int udaClientGetHostSSL(int id);
LIBRARY_API int udaClientGetHostNameId();
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_HOSTLIST_H
