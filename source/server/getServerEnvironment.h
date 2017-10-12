#ifndef IDAM_SERVER_GETSERVERENVIRONMENT_H
#define IDAM_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>

void printIdamServerEnvironment(const ENVIRONMENT* environ);
ENVIRONMENT* getIdamServerEnvironment();

void putIdamServerEnvironment(const ENVIRONMENT* environ);

#endif // IDAM_SERVER_GETSERVERENVIRONMENT_H
