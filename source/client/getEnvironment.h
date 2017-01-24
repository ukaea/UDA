
#ifndef IDAM_GETENVIRONMENT_H
#define IDAM_GETENVIRONMENT_H

#include "idamclientserver.h"
#include "idamclient.h"

#ifdef FATCLIENT
#  define printIdamClientEnvironment printIdamClientEnvironmentFat
#  define getIdamClientEnvironment getIdamClientEnvironmentFat
#endif

void printIdamClientEnvironment(ENVIRONMENT * environ);
void getIdamClientEnvironment(ENVIRONMENT * environ);

#endif // IDAM_GETENVIRONMENT_H

