#ifndef UDA_CLIENT_GETENVIRONMENT_H
#define UDA_CLIENT_GETENVIRONMENT_H

#include <clientserver/udaStructs.h>

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#  define putIdamClientEnvironment putIdamClientEnvironmentFat
#endif

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void printIdamClientEnvironment(const ENVIRONMENT* environment);
LIBRARY_API ENVIRONMENT* getIdamClientEnvironment();
LIBRARY_API void putIdamClientEnvironment(const ENVIRONMENT* environment);

extern int env_host;            // Flags to Read Environment variable at startup time
extern int env_port;

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_GETENVIRONMENT_H