#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#include "export.h"
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaStartup(int reset, CLIENT_FLAGS* client_flags, bool* reopen_logs);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_STARTUP_H
