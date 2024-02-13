#pragma once

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#include "udaClient.h"

namespace uda::client
{

int udaStartup(int reset, CLIENT_FLAGS* client_flags, bool* reopen_logs);

}
