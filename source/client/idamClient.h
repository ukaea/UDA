#ifndef IDAM_CLIENT_IDAMCLIENT_H
#define IDAM_CLIENT_IDAMCLIENT_H

#include <clientserver/idamStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamClient idamClientFat
#  define idamFreeAll idamFreeAllFat
#endif

#define MIN_STATUS          -1      // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD     -17000  // Error Code if Status is Bad

extern int initEnvironment;
extern int altRank;

extern int malloc_source;

int idamClient(REQUEST_BLOCK * request_block);
void updateClientBlock(CLIENT_BLOCK* str);
void idamFree(int handle);
void idamFreeAll();

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENT_IDAMCLIENT_H
