#ifndef UDA_SERVER_SLEEPSERVER_H
#define UDA_SERVER_SLEEPSERVER_H

#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int sleepServer(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SLEEPSERVER_H

