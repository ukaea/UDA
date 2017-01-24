#ifndef IDAM_IDAMAPI3_H
#define IDAM_IDAMAPI3_H

#ifdef MEMDEBUG
#include <mcheck.h>
#endif

#define SENTINEL	-32768
#define idamGetAPI2(target, ...) idamGetAPI2A(target, __VA_ARGS__, SENTINEL)

#ifdef __cplusplus
extern "C" {
#endif

extern int idamGetAPI2A(const char *target, ...);

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#endif

int idamGetAPI(const char *data_object, const char *data_source);

//! Legacy API - \b do \b not \b use!
int idamAPI(const char *signal, int exp_number);

//! Legacy API - \b do \b not \b use!
int idamPassAPI(const char *signal, int exp_number, int pass);

//! Legacy API - \b do \b not \b use!
int idamGenAPI(const char *archive, const char *device, const char *signal, int exp_number, int pass);

#ifdef __cplusplus
}
#endif

#endif // IDAM_IDAMAPI3_H

