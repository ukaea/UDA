#pragma once

#include "clientserver/udaStructs.h"

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

namespace uda::client
{

void printIdamClientEnvironment(const uda::client_server::Environment* environment);

uda::client_server::Environment* getIdamClientEnvironment();

void putIdamClientEnvironment(const uda::client_server::Environment* environment);

bool udaGetEnvHost();

bool udaGetEnvPort();

void udaSetEnvHost(bool env_host);

void udaSetEnvPort(bool env_port);

} // namespace uda::client
