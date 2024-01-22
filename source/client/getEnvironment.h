#ifndef UDA_CLIENT_GETENVIRONMENT_H
#define UDA_CLIENT_GETENVIRONMENT_H

#include "export.h"
#include "udaStructs.h"

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void printIdamClientEnvironment(const ENVIRONMENT* environment);
LIBRARY_API ENVIRONMENT* getIdamClientEnvironment();
LIBRARY_API void putIdamClientEnvironment(const ENVIRONMENT* environment);

LIBRARY_API bool udaGetEnvHost();
LIBRARY_API bool udaGetEnvPort();
LIBRARY_API void udaSetEnvHost(bool env_host);
LIBRARY_API void udaSetEnvPort(bool env_port);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_GETENVIRONMENT_H