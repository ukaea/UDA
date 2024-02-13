#ifndef UDA_CLIENT_GETENVIRONMENT_H
#define UDA_CLIENT_GETENVIRONMENT_H

#include "clientserver/udaStructs.h"

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

void printIdamClientEnvironment(const uda::client_server::ENVIRONMENT* environment);
uda::client_server::ENVIRONMENT* getIdamClientEnvironment();
void putIdamClientEnvironment(const uda::client_server::ENVIRONMENT* environment);

bool udaGetEnvHost();
bool udaGetEnvPort();
void udaSetEnvHost(bool env_host);
void udaSetEnvPort(bool env_port);

#endif // UDA_CLIENT_GETENVIRONMENT_H