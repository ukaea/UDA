#ifndef IDAM_CLIENTAPI_H
#define IDAM_CLIENTAPI_H

#include "idamclientserverpublic.h"
#include "idamclientserverprivate.h"
#include "idamclientpublic.h"
#include "idamclientprivate.h"

int idamClientAPI(const char *file, const char *signal, int pass, int exp_number);
int idamClientFileAPI(const char *file, const char *signal, const char *format);
int idamClientFileAPI2(const char *file, const char *format, const char *owner,
                       const char *signal, int exp_number, int pass);
int idamClientTestAPI(const char *file, const char *signal, int pass, int exp_number);

#endif // IDAM_CLIENTAPI_H
