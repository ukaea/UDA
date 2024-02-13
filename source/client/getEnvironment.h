#pragma once

#include "clientserver/udaStructs.h"

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

namespace uda::client {

void printIdamClientEnvironment(const uda::client_server::ENVIRONMENT *environment);

uda::client_server::ENVIRONMENT *getIdamClientEnvironment();

void putIdamClientEnvironment(const uda::client_server::ENVIRONMENT *environment);

bool udaGetEnvHost();

bool udaGetEnvPort();

void udaSetEnvHost(bool env_host);

void udaSetEnvPort(bool env_port);

}
