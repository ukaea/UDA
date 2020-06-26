#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#define HOST_MCOUNT		100		// Maximum initial number of hosts that can be registered
#define HOST_MSTEP		10		// Increase heap by 10 records once the maximum is exceeded
#define HOST_STRING		256

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HostData {
    char hostalias[HOST_STRING];
    char hostname[HOST_STRING];
    char certificate[HOST_STRING];
    char key[HOST_STRING];
    char ca_certificate[HOST_STRING];
    int port;
    int isSSL;
} HOSTDATA;

LIBRARY_API void udaClientAllocHostList(int count);
LIBRARY_API void udaClientFreeHostList(void);
LIBRARY_API void udaClientInitHostData(HOSTDATA* host);
LIBRARY_API int udaClientFindHostByAlias(const char* alias);
LIBRARY_API int udaClientFindHostByName(const char* name);
LIBRARY_API void udaClientInitHostList(void);
LIBRARY_API char* udaClientGetHostName(int id);
LIBRARY_API char* udaClientGetHostAlias(int id);
LIBRARY_API int udaClientGetHostPort(int id);
LIBRARY_API char* udaClientGetHostCertificatePath(int id);
LIBRARY_API char* udaClientGetHostKeyPath(int id);
LIBRARY_API char* udaClientGetHostCAPath(int id);
LIBRARY_API int udaClientGetHostSSL(int id);
LIBRARY_API void udaClientPutHostNameId(int id);
LIBRARY_API int udaClientGetHostNameId(void);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_HOSTLIST_H
