#ifndef IDAM_CLIENT_CLIENTAPI_H
#define IDAM_CLIENT_CLIENTAPI_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int idamClientAPI(const char *file, const char *signal, int pass, int exp_number);
int idamClientFileAPI(const char *file, const char *signal, const char *format);
int idamClientFileAPI2(const char *file, const char *format, const char *owner,
                       const char *signal, int exp_number, int pass);
int idamClientTestAPI(const char *file, const char *signal, int pass, int exp_number);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENT_CLIENTAPI_H
