#ifndef IDAM_SERVER_GETSERVERENVIRONMENT_H
#define IDAM_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void printIdamServerEnvironment(const ENVIRONMENT* environ);
ENVIRONMENT* getIdamServerEnvironment();

void putIdamServerEnvironment(const ENVIRONMENT* environ);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_GETSERVERENVIRONMENT_H
