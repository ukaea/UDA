#ifndef UDA_SERVER_GETSERVERENVIRONMENT_H
#define UDA_SERVER_GETSERVERENVIRONMENT_H

#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void printIdamServerEnvironment(const ENVIRONMENT* environment);
LIBRARY_API ENVIRONMENT* getIdamServerEnvironment();

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_GETSERVERENVIRONMENT_H
