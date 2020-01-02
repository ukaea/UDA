#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void printIdamServerEnvironment(const ENVIRONMENT* environment);
ENVIRONMENT* getIdamServerEnvironment();

void putIdamServerEnvironment(const ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
