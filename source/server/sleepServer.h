#ifndef UDA_SERVER_SLEEPSERVER_H
#define UDA_SERVER_SLEEPSERVER_H

#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int sleepServer(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SLEEPSERVER_H
