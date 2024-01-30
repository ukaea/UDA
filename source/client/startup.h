#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#include "udaClient.h"

int udaStartup(int reset, CLIENT_FLAGS* client_flags, bool* reopen_logs);

#endif // UDA_CLIENT_STARTUP_H
