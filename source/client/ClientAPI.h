#ifndef IDAM_CLIENT_CLIENTAPI_H
#define IDAM_CLIENT_CLIENTAPI_H

int idamClientAPI(const char *file, const char *signal, int pass, int exp_number);
int idamClientFileAPI(const char *file, const char *signal, const char *format);
int idamClientFileAPI2(const char *file, const char *format, const char *owner,
                       const char *signal, int exp_number, int pass);
int idamClientTestAPI(const char *file, const char *signal, int pass, int exp_number);

#endif // IDAM_CLIENT_CLIENTAPI_H
