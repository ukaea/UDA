#ifndef IDAM_CLIENT_GETENVIRONMENT_H
#define IDAM_CLIENT_GETENVIRONMENT_H

#include <include/idamclientserverprivate.h>

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#endif

void printIdamClientEnvironment(ENVIRONMENT * environ);
void getIdamClientEnvironment(ENVIRONMENT * environ);

#endif // IDAM_CLIENT_GETENVIRONMENT_H

