#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void printServerEnvironment(const ENVIRONMENT* environment);
LIBRARY_API ENVIRONMENT* getServerEnvironment();

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
