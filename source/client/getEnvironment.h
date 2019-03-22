#ifndef UDA_CLIENT_GETENVIRONMENT_H
#define UDA_CLIENT_GETENVIRONMENT_H

#include <clientserver/udaStructs.h>

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

void printIdamClientEnvironment(const ENVIRONMENT* environment);
ENVIRONMENT* getIdamClientEnvironment();
void putIdamClientEnvironment(const ENVIRONMENT* environment);

extern int env_host;            // Flags to Read Environment variable at startup time
extern int env_port;

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_GETENVIRONMENT_H