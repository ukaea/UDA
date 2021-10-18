#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaStartup(int reset, int* alt_rank, CLIENT_FLAGS* client_flags);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_STARTUP_H
