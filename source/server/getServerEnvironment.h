#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

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

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
