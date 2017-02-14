#ifndef IDAM_CLIENT_GETENVIRONMENT_H
#define IDAM_CLIENT_GETENVIRONMENT_H

#include <clientserver/idamStructs.h>

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#endif

void printIdamClientEnvironment(ENVIRONMENT* environ);
void getIdamClientEnvironment(ENVIRONMENT* environ);

extern int env_host;            // Flags to Read Environment variable at startup time
extern int env_port;

#endif // IDAM_CLIENT_GETENVIRONMENT_H

