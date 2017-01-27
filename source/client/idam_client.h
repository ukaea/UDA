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

int idamClient(REQUEST_BLOCK * request_block);
void updateClientBlock(CLIENT_BLOCK* str);
void idamFree(int handle);
void idamFreeAll();

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENT_IDAMCLIENT_H
