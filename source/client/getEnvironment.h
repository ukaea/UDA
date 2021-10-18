#ifndef UDA_CLIENT_GETENVIRONMENT_H
#define UDA_CLIENT_GETENVIRONMENT_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

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

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_GETENVIRONMENT_H