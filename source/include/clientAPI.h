#ifndef UDA_CLIENT_CLIENTAPI_H
#define UDA_CLIENT_CLIENTAPI_H

#include "export.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaClientAPI(const char* file, const char* signal, int pass, int exp_number);
LIBRARY_API int udaClientFileAPI(const char* file, const char* signal, const char* format);
LIBRARY_API int udaClientFileAPI2(const char* file, const char* format, const char* owner, const char* signal,
                                  int exp_number, int pass);
LIBRARY_API int udaClientTestAPI(const char* file, const char* signal, int pass, int exp_number);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLIENTAPI_H
